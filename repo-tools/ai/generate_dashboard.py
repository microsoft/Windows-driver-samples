#!/usr/bin/env python3
"""
generate_dashboard.py — Generates an Issues & PR dashboard for Windows-driver-samples.

Parses .github/CODEOWNERS to build team-to-path mappings, reads GitHub issues
and PRs from JSON files (fetched separately), classifies each item to exactly
ONE team, and outputs a self-contained HTML dashboard.

Usage:
    python generate_dashboard.py [--issues ISSUES_JSON] [--prs PRS_JSON] [--codeowners PATH] [--output PATH]

Defaults:
    --issues    _data/issues.json
    --prs       _data/prs.json
    --codeowners ../../.github/CODEOWNERS
    --output    output/dashboard.html
"""

import argparse
import json
import html as html_mod
import os
import re
import sys
from datetime import datetime, timezone
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent


# ---------------------------------------------------------------------------
# 1. Parse CODEOWNERS
# ---------------------------------------------------------------------------
def parse_codeowners(codeowners_path: str) -> tuple[dict[str, list[str]], dict[str, str], list[tuple[str, str]]]:
    """
    Returns:
      teams:    {team_alias: [path1, path2, ...]}
      sections: {team_alias: "Section Comment Name"}  (from # comments above entries)
      rules:    [(pattern, team_alias), ...] in file order (last match wins, like GitHub)
    """
    teams: dict[str, list[str]] = {}
    sections: dict[str, str] = {}
    rules: list[tuple[str, str]] = []
    current_comment = ""
    with open(codeowners_path, "r", encoding="utf-8") as f:
        for line in f:
            stripped = line.strip()
            if stripped.startswith("#"):
                current_comment = stripped.lstrip("# ").strip()
                continue
            if not stripped:
                continue
            parts = stripped.split()
            if len(parts) < 2:
                continue
            path_pattern = parts[0]
            owner = parts[1]  # e.g. @microsoft/bluetooth
            alias = owner.lstrip("@").split("/", 1)[-1]  # -> bluetooth
            teams.setdefault(alias, []).append(path_pattern)
            rules.append((path_pattern, alias))
            if current_comment and alias not in sections:
                sections[alias] = current_comment
    return teams, sections, rules


# ---------------------------------------------------------------------------
# CODEOWNERS path matcher — mirrors GitHub's matching logic
# ---------------------------------------------------------------------------
def _codeowners_pattern_to_regex(pattern: str) -> re.Pattern:
    """
    Convert a CODEOWNERS glob pattern to a regex.
    Rules (matching GitHub behavior):
      - Leading / anchors to repo root; strip it for regex
      - * matches anything except /
      - ** matches anything including /
      - Trailing / means directory and everything inside
    """
    p = pattern
    if p.startswith("/"):
        p = p[1:]
    if p.endswith("/"):
        p = p + "**"

    parts = []
    i = 0
    while i < len(p):
        if p[i:i+2] == "**":
            parts.append(".*")
            i += 2
            if i < len(p) and p[i] == "/":
                i += 1
        elif p[i] == "*":
            parts.append("[^/]*")
            i += 1
        elif p[i] == "?":
            parts.append("[^/]")
            i += 1
        else:
            parts.append(re.escape(p[i]))
            i += 1

    regex_str = "^" + "".join(parts)
    if not pattern.endswith("*") and not pattern.endswith("/"):
        regex_str += "(/.*)?$"
    else:
        regex_str += "$"

    return re.compile(regex_str, re.IGNORECASE)


def match_path_to_team(file_path: str, rules: list[tuple[str, str]]) -> str | None:
    """
    Given a repo-relative file path, find the owning team using CODEOWNERS rules.
    Last matching rule wins (GitHub behavior).
    """
    # Normalize: strip leading / or ./
    fp = file_path
    if fp.startswith("./"):
        fp = fp[2:]
    fp = fp.lstrip("/")
    matched_team = None
    for pattern, alias in rules:
        regex = _codeowners_pattern_to_regex(pattern)
        if regex.match(fp):
            matched_team = alias
    return matched_team


# ---------------------------------------------------------------------------
# File/folder path extraction from issue/PR text
# ---------------------------------------------------------------------------
_REPO_NAMES = ["windows-driver-samples", "Windows-driver-samples"]
_FILE_EXT_RE = r'\.(?:ps1|c|cpp|h|hpp|inf|vcxproj|sln|yml|yaml|md|txt|rc|def|inx|props|targets|csv|json|xml|mof)'

_KNOWN_TOP_DIRS = [
    "audio", "avstream", "bluetooth", "filesys", "general", "gnss", "gpio",
    "hid", "input", "network", "nfc", "nfp", "pofx", "pos", "powerlimit",
    "print", "prm", "sd", "security", "sensors", "serial", "setup", "simbatt",
    "smartcrd", "spb", "storage", "thermal", "tools", "TrEE", "usb", "video",
    "wia", "wil", "wmi", "repo-tools",
]


def extract_paths(title: str, body: str | None) -> list[str]:
    """
    Extract file and folder paths mentioned in issue/PR text.

    Strategies:
      1. GitHub blob/tree URLs → repo-relative path
      2. Paths with known file extensions (e.g. Build-SampleSet.ps1)
      3. .github/ references (actions, workflows, CODEOWNERS)
      4. Known top-level directory paths (e.g. network/trans/WFPSampler)
      5. Title-embedded tree paths (e.g. Windows-driver-samples/tree/main/...)

    Returns deduplicated repo-relative paths (no leading /).
    """
    text = (title or "") + "\n" + (body or "")
    paths: list[str] = []

    # 1. GitHub blob/tree URLs — ONLY from this repo (microsoft/Windows-driver-samples)
    github_url_re = re.compile(
        r'github\.com/microsoft/(?:' + '|'.join(re.escape(n) for n in _REPO_NAMES) + r')'
        r'/(?:blob|tree)/[^/]+/([^\s#"\'<>\)]+)',
        re.IGNORECASE,
    )
    for m in github_url_re.finditer(text):
        paths.append(m.group(1))

    # 2. Paths with file extensions
    file_path_re = re.compile(
        r'(?:^|[\s`"\'(])([A-Za-z0-9_.][A-Za-z0-9_./\\-]*' + _FILE_EXT_RE + r')',
        re.MULTILINE,
    )
    for m in file_path_re.finditer(text):
        candidate = m.group(1).replace("\\", "/").strip()
        if ":" in candidate or candidate.startswith("http"):
            continue
        paths.append(candidate)

    # 3. .github/ references — only from this repo's context
    github_dir_re = re.compile(r'(?:^|[\s`"\'(/])(\.[Gg]ithub/[^\s"\'<>\)]*)', re.MULTILINE)
    for m in github_dir_re.finditer(text):
        match_start = m.start()
        preceding = text[max(0, match_start - 80):match_start]
        if "github.com/" not in preceding or "Windows-driver-samples" in preceding:
            paths.append(m.group(1))

    # 4. GitHub Actions / CI references — only if title suggests this repo's CI
    title_lower = (title or "").lower()
    if any(kw in title_lower for kw in ["actions", "codeql", "workflow", "ci/cd", "pin "]):
        paths.append(".github/workflows")

    # 5. Known top-level directory paths — skip if inside a URL from another repo
    dir_re = re.compile(
        r'(?:^|[\s`"\'(/])('
        + "|".join(re.escape(d) for d in _KNOWN_TOP_DIRS)
        + r')/([A-Za-z0-9_./\\-]+)',
        re.IGNORECASE | re.MULTILINE,
    )
    for m in dir_re.finditer(text):
        match_start = m.start()
        preceding = text[max(0, match_start - 100):match_start]
        if "github.com/" in preceding and "Windows-driver-samples" not in preceding:
            continue
        paths.append(m.group(1) + "/" + m.group(2))

    # 6. Title contains "Windows-driver-samples/tree/..." pattern
    title_tree_re = re.compile(
        r'Windows-driver-samples/tree/[^/]+/(.+)', re.IGNORECASE
    )
    tm = title_tree_re.search(title or "")
    if tm:
        paths.append(tm.group(1))

    # Dedupe and normalize
    seen: set[str] = set()
    result: list[str] = []
    for p in paths:
        p = p.replace("\\", "/").strip("/").rstrip(".")
        # Strip tree/branch prefix if accidentally captured
        tree_prefix = re.match(r'^tree/[^/]+/(.*)', p)
        if tree_prefix:
            p = tree_prefix.group(1)
        if p and p not in seen:
            seen.add(p)
            result.append(p)
    return result



def _normalize(text: str) -> str:
    """Strip spaces, hyphens, underscores, slashes for fuzzy matching."""
    return re.sub(r'[\s\-_/\\.]', '', text.lower())


def _split_compound(word: str) -> list[str]:
    """
    Split a compound path-leaf like 'wmiacpi' into possible sub-words
    by inserting splits at known driver-domain boundaries.
    """
    boundaries = [
        ('wmi', 'acpi'), ('wmi', 'samp'), ('system', 'dma'),
        ('simple', 'media', 'source'), ('simple', 'audio', 'sample'),
        ('net', 'vmini'), ('fast', 'fat'), ('mini', 'filter'),
        ('bth', 'echo'), ('usb', 'view'), ('pcm', 'drv'),
        ('pci', 'drv'), ('sim', 'batt'), ('power', 'limit'),
        ('perf', 'counters'), ('smart', 'crd'),
    ]
    lower = word.lower()
    for parts in boundaries:
        if ''.join(parts) == lower:
            return [' '.join(parts)]
    return []


# Minimum length for plain substring matching. Keywords shorter than this
# require word-boundary matching to avoid false positives (e.g. "pos" in "posted").
_MIN_SUBSTR_LEN = 4

# Words too generic to be useful as classification keywords — these appear across
# many unrelated issues/PRs and cause false positives.
_STOPWORDS = frozenset({
    "general", "common", "samples", "sample", "driver", "drivers", "test",
    "tests", "testing", "src", "inc", "lib", "docs", "readme", "build",
    "change", "changes", "delete", "arm64", "x64", "x86", "amd64",
    "func", "function", "data", "util", "utils", "config", "core",
    "main", "app", "info", "help", "log", "logs", "debug", "release",
    "shared", "public", "private", "internal", "version", "update",
    "network", "filesys", "pofx", "wmi",  # ambiguous parent dirs shared by multiple teams
})


def _kw_matches_raw(kw: str, raw_text: str) -> bool:
    """
    Check if keyword matches in raw text. Short keywords (< _MIN_SUBSTR_LEN)
    use word-boundary regex to prevent false positives.
    """
    if len(kw) < _MIN_SUBSTR_LEN:
        return bool(re.search(r'\b' + re.escape(kw) + r'\b', raw_text))
    return kw in raw_text


def _scan_subdirs(repo_root: str, owned_paths: list[str]) -> list[str]:
    """
    Scan the actual repo directory to find subdirectory names under each
    CODEOWNERS-owned path. This captures sample names like 'KMDOD' under /video/.
    Returns lowercased subdirectory names, filtered against stopwords.
    """
    extra = []
    for p in owned_paths:
        clean = p.strip("/")
        if clean == "*" or clean.startswith("."):
            continue
        full = os.path.join(repo_root, clean.replace("/", os.sep))
        if os.path.isdir(full):
            try:
                for entry in os.listdir(full):
                    entry_path = os.path.join(full, entry)
                    if os.path.isdir(entry_path) and not entry.startswith("."):
                        name = entry.lower()
                        if name not in _STOPWORDS and len(name) >= _MIN_SUBSTR_LEN:
                            extra.append(name)
            except OSError:
                pass
    return extra


def _is_useful_keyword(kw: str) -> bool:
    """Filter out keywords that are stopwords or too generic."""
    return kw not in _STOPWORDS and len(kw) >= 3


def build_keywords(
    teams: dict[str, list[str]], sections: dict[str, str],
    repo_root: str | None = None,
    extra_keywords_path: str | None = None,
) -> dict[str, list[str]]:
    """
    For each team, build a list of searchable keywords derived from:
      1. CODEOWNERS path entries (full path, leaf segment, normalized variants)
      2. Section comment names (e.g. "Device Enumeration and Interconnect")
      3. Subdirectory names under owned paths (if repo_root is provided)
      4. Manual overrides from extra_keywords.json (if provided)

    Keywords are filtered against a stopword list to prevent false positives
    from generic terms like "general", "common", "test", etc.
    """
    # Load manual keyword overrides
    extra_kw: dict[str, list[str]] = {}
    if extra_keywords_path and os.path.isfile(extra_keywords_path):
        with open(extra_keywords_path, "r", encoding="utf-8") as f:
            extra_kw = json.load(f)
        print(f"   Loaded extra keywords for {len(extra_kw)} teams from {extra_keywords_path}")

    keywords: dict[str, list[str]] = {}
    for alias, paths in teams.items():
        kws: list[str] = []
        for p in paths:
            clean = p.strip("/")
            if clean == "*":
                continue
            # Full path as-is (e.g. "network/trans") — always useful regardless of stopwords
            kws.append(clean.lower())
            # Leaf segment (e.g. "trans")
            leaf = clean.rsplit("/", 1)[-1].lower()
            if leaf and leaf != "*" and _is_useful_keyword(leaf):
                kws.append(leaf)
                for variant in _split_compound(leaf):
                    kws.append(variant)

        # Subdirectory names from the actual repo tree
        if repo_root:
            for subdir in _scan_subdirs(repo_root, paths):
                kws.append(subdir)

        # Section comment as keyword
        if alias in sections:
            comment = sections[alias].lower()
            if comment:
                kws.append(comment)

        # Manual overrides from extra_keywords.json
        if alias in extra_kw:
            kws.extend(k.lower() for k in extra_kw[alias])

        keywords[alias] = list(dict.fromkeys(kws))  # dedupe, preserve order
    return keywords


# ---------------------------------------------------------------------------
# 2. Classify items — exactly ONE team per item
# ---------------------------------------------------------------------------
def classify_one(
    title: str,
    body: str | None,
    team_keywords: dict[str, list[str]],
    codeowners_rules: list[tuple[str, str]] | None = None,
) -> tuple[str, str | None]:
    """
    Assign exactly one team to an issue/PR.

    Returns (team_alias, detected_path_or_None).

    Four-pass strategy (first match with highest specificity wins):
      Pass 0 (highest): Extract file/folder paths from text, match them
              against CODEOWNERS rules. Most specific (longest) path wins.
              This is the most reliable signal — it's exactly how GitHub
              assigns code owners.
      Pass 1: Direct keyword match on raw text — short keywords use
              word-boundary regex to prevent false positives.
      Pass 2: Normalized match — strip whitespace/separators.
      Pass 3: Default to 'driver-samples-maintainers'.
    """
    # --- Pass 0: Path-based CODEOWNERS matching ---
    detected_path = None
    if codeowners_rules:
        extracted = extract_paths(title, body)
        best_path_team = None
        best_path_len = 0
        best_path = None
        for fp in extracted:
            team = match_path_to_team(fp, codeowners_rules)
            if team and len(fp) > best_path_len:
                best_path_team = team
                best_path_len = len(fp)
                best_path = fp
        if best_path_team:
            detected_path = best_path
            return best_path_team, detected_path

    # --- Pass 1 & 2: Keyword-based fallback ---
    raw_text = ((title or "") + " " + (body or "")).lower()
    norm_text = _normalize(raw_text)

    best_team = None
    best_len = 0
    best_pass = 99

    for alias, kws in team_keywords.items():
        if alias == "driver-samples-maintainers":
            continue
        for kw in kws:
            kw_lower = kw.lower()

            # Pass 1: match in raw text (word-boundary for short keywords)
            if _kw_matches_raw(kw_lower, raw_text):
                if best_pass > 1 or (best_pass == 1 and len(kw_lower) > best_len):
                    best_team = alias
                    best_len = len(kw_lower)
                    best_pass = 1
                continue

            # Pass 2: normalized match
            norm_kw = _normalize(kw_lower)
            if len(norm_kw) >= _MIN_SUBSTR_LEN and norm_kw in norm_text:
                if best_pass > 2 or (best_pass == 2 and len(norm_kw) > best_len):
                    best_team = alias
                    best_len = len(norm_kw)
                    best_pass = 2

    return (best_team or "driver-samples-maintainers"), detected_path


# ---------------------------------------------------------------------------
# 3. Load data
# ---------------------------------------------------------------------------
def load_issues(path: str) -> list[dict]:
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    if isinstance(data, dict) and "issues" in data:
        return data["issues"]
    return data


def load_prs(path: str) -> list[dict]:
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


# ---------------------------------------------------------------------------
# 4. HTML generation
# ---------------------------------------------------------------------------
PALETTE = [
    "#0078d4", "#107c10", "#d83b01", "#5c2d91", "#008272",
    "#b4009e", "#004e8c", "#e3008c", "#986f0b", "#498205",
    "#00b7c3", "#ca5010", "#4f6bed", "#847545", "#6b69d6",
    "#038387", "#c30052", "#7a7574", "#0063b1", "#69797e",
    "#744da9", "#da3b01", "#107c10", "#767676", "#c239b3",
    "#00cc6a", "#ff8c00", "#e81123",
]

LABEL_COLORS = {
    "bug": "#d73a4a",
    "question": "#d876e3",
    "request": "#0075ca",
    "help wanted": "#008672",
}


def generate_dashboard(
    classified_issues: list[dict],
    classified_prs: list[dict],
    all_teams: list[str],
    repo_url: str,
    output_path: str,
):
    team_colors = {}
    for idx, team in enumerate(sorted(all_teams)):
        team_colors[team] = PALETTE[idx % len(PALETTE)]

    def badge(team):
        c = team_colors.get(team, "#6a737d")
        return f'<span class="team-badge" style="background:{c}">{html_mod.escape(team)}</span>'

    def label_span(lbl):
        c = LABEL_COLORS.get(lbl, "#6a737d")
        return f'<span class="label" style="background:{c}">{html_mod.escape(lbl)}</span>'

    def item_row(item):
        labels_html = " ".join(label_span(l) for l in item["labels"])
        dp = item.get("detected_path")
        path_html = (
            f'<span class="detected-path" title="Matched via CODEOWNERS">📁 {html_mod.escape(dp)}</span>'
            if dp else '<span class="no-path">keyword match</span>'
        )
        return (
            f'<tr class="item-row" data-team="{html_mod.escape(item["team"])}">'
            f'<td><a href="{html_mod.escape(item["url"])}" target="_blank">#{item["number"]}</a></td>'
            f'<td class="title-cell"><span class="item-title">{html_mod.escape(item["title"])}</span>'
            f'<br><span class="meta">by {html_mod.escape(item["user"])}</span></td>'
            f"<td>{badge(item['team'])}</td>"
            f"<td>{path_html}</td>"
            f"<td>{labels_html}</td></tr>"
        )

    # Team summary
    team_issue_count = {}
    team_pr_count = {}
    for t in all_teams:
        team_issue_count[t] = sum(1 for i in classified_issues if i["team"] == t)
        team_pr_count[t] = sum(1 for p in classified_prs if p["team"] == t)

    # Cards
    cards_html = ""
    for team in sorted(all_teams):
        ni = team_issue_count[team]
        np_ = team_pr_count[team]
        if ni == 0 and np_ == 0:
            continue
        c = team_colors.get(team, "#6a737d")
        cards_html += f"""
    <div class="team-card" onclick="filterByTeam('{html_mod.escape(team)}')">
      <div class="card-color" style="background:{c}"></div>
      <div class="card-body">
        <div class="card-title">{html_mod.escape(team)}</div>
        <div class="card-stats">
          <span class="stat">🔴 {ni} issue{"s" if ni != 1 else ""}</span>
          <span class="stat">🟢 {np_} PR{"s" if np_ != 1 else ""}</span>
        </div>
      </div>
    </div>"""

    # Filter buttons
    team_buttons = '<button class="filter-btn active" onclick="filterByTeam(\'all\')">All Teams</button>\n'
    for team in sorted(all_teams):
        if team_issue_count.get(team, 0) + team_pr_count.get(team, 0) == 0:
            continue
        c = team_colors.get(team, "#6a737d")
        team_buttons += (
            f'<button class="filter-btn" data-team="{html_mod.escape(team)}" '
            f"onclick=\"filterByTeam('{html_mod.escape(team)}')\" "
            f'style="border-left:3px solid {c}">{html_mod.escape(team)}</button>\n'
        )

    issue_rows = "\n".join(
        item_row(i) for i in sorted(classified_issues, key=lambda x: x["number"], reverse=True)
    )
    pr_rows = "\n".join(
        item_row(p) for p in sorted(classified_prs, key=lambda x: x["number"], reverse=True)
    )

    now_str = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")

    dashboard_html = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Windows Driver Samples — Issues &amp; PR Dashboard</title>
<style>
  :root {{
    --bg: #0d1117; --surface: #161b22; --border: #30363d;
    --text: #e6edf3; --text-muted: #8b949e; --accent: #58a6ff;
  }}
  * {{ margin:0; padding:0; box-sizing:border-box; }}
  body {{ font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Helvetica, Arial, sans-serif; background:var(--bg); color:var(--text); }}
  .container {{ max-width:1400px; margin:0 auto; padding:24px; }}
  header {{ text-align:center; padding:32px 0 24px; border-bottom:1px solid var(--border); margin-bottom:24px; }}
  header h1 {{ font-size:28px; font-weight:600; }}
  header p {{ color:var(--text-muted); margin-top:8px; font-size:14px; }}
  header a {{ color:var(--accent); }}
  .summary-bar {{ display:flex; gap:16px; justify-content:center; margin:16px 0 24px; flex-wrap:wrap; }}
  .summary-box {{ background:var(--surface); border:1px solid var(--border); border-radius:8px; padding:16px 28px; text-align:center; }}
  .summary-box .big {{ font-size:32px; font-weight:700; }}
  .summary-box .lbl {{ color:var(--text-muted); font-size:13px; margin-top:2px; }}
  .section-title {{ font-size:20px; font-weight:600; margin:28px 0 12px; border-bottom:1px solid var(--border); padding-bottom:8px; }}
  .team-grid {{ display:grid; grid-template-columns:repeat(auto-fill,minmax(280px,1fr)); gap:12px; margin-bottom:28px; }}
  .team-card {{ background:var(--surface); border:1px solid var(--border); border-radius:8px; display:flex; overflow:hidden; cursor:pointer; transition:transform .15s,box-shadow .15s; }}
  .team-card:hover {{ transform:translateY(-2px); box-shadow:0 4px 12px rgba(0,0,0,.4); }}
  .card-color {{ width:5px; flex-shrink:0; }}
  .card-body {{ padding:14px 16px; }}
  .card-title {{ font-weight:600; font-size:14px; margin-bottom:6px; font-family:monospace; }}
  .card-stats {{ display:flex; gap:14px; }}
  .stat {{ font-size:13px; color:var(--text-muted); }}
  .filter-bar {{ display:flex; flex-wrap:wrap; gap:6px; margin-bottom:16px; }}
  .filter-btn {{ background:var(--surface); color:var(--text); border:1px solid var(--border); border-radius:6px; padding:5px 14px; font-size:12px; cursor:pointer; transition:all .15s; font-family:monospace; }}
  .filter-btn:hover {{ background:#21262d; }}
  .filter-btn.active {{ background:var(--accent); color:#000; border-color:var(--accent); font-weight:600; }}
  .search-box {{ margin-bottom:16px; }}
  .search-box input {{ width:100%; padding:10px 14px; background:var(--surface); border:1px solid var(--border); border-radius:6px; color:var(--text); font-size:14px; outline:none; }}
  .search-box input:focus {{ border-color:var(--accent); }}
  /* Tabs */
  .tab-bar {{ display:flex; gap:0; margin-bottom:0; border-bottom:2px solid var(--border); }}
  .tab {{ padding:12px 28px; font-size:15px; font-weight:600; cursor:pointer; border-bottom:2px solid transparent; margin-bottom:-2px; color:var(--text-muted); transition:all .15s; }}
  .tab:hover {{ color:var(--text); }}
  .tab.active {{ color:var(--accent); border-bottom-color:var(--accent); }}
  .tab-content {{ display:none; padding-top:20px; }}
  .tab-content.active {{ display:block; }}
  table {{ width:100%; border-collapse:collapse; }}
  table th {{ text-align:left; font-size:12px; color:var(--text-muted); padding:8px 10px; border-bottom:1px solid var(--border); }}
  table td {{ padding:10px; border-bottom:1px solid var(--border); font-size:13px; vertical-align:top; }}
  table td a {{ color:var(--accent); text-decoration:none; }}
  table td a:hover {{ text-decoration:underline; }}
  .title-cell {{ max-width:500px; }}
  .item-title {{ font-weight:500; }}
  .meta {{ color:var(--text-muted); font-size:12px; }}
  .label {{ display:inline-block; padding:2px 8px; border-radius:12px; font-size:11px; font-weight:600; color:#fff; margin:1px 2px; }}
  .team-badge {{ display:inline-block; padding:2px 8px; border-radius:12px; font-size:11px; font-weight:500; color:#fff; margin:1px 2px; font-family:monospace; }}
  .detected-path {{ font-family:monospace; font-size:12px; color:#7ee787; background:#1b2a1b; padding:2px 6px; border-radius:4px; }}
  .no-path {{ font-size:11px; color:var(--text-muted); font-style:italic; }}
  .hidden {{ display:none; }}
  .counter {{ color:var(--text-muted); font-size:14px; margin-bottom:8px; }}
  footer {{ text-align:center; padding:32px 0; color:var(--text-muted); font-size:12px; border-top:1px solid var(--border); margin-top:32px; }}
</style>
</head>
<body>
<div class="container">
  <header>
    <h1>📊 Windows Driver Samples Dashboard</h1>
    <p>Open Issues &amp; Pull Requests by CODEOWNERS Team — <a href="{html_mod.escape(repo_url)}" target="_blank">{html_mod.escape(repo_url.replace("https://github.com/",""))}</a></p>
  </header>

  <div class="summary-bar">
    <div class="summary-box"><div class="big">{len(classified_issues)+len(classified_prs)}</div><div class="lbl">Total Open Items</div></div>
    <div class="summary-box"><div class="big" style="color:#d73a4a">{len(classified_issues)}</div><div class="lbl">Open Issues</div></div>
    <div class="summary-box"><div class="big" style="color:#3fb950">{len(classified_prs)}</div><div class="lbl">Open PRs</div></div>
    <div class="summary-box"><div class="big" style="color:var(--accent)">{len([t for t in all_teams if team_issue_count.get(t,0)+team_pr_count.get(t,0)>0])}</div><div class="lbl">Active Teams</div></div>
  </div>

  <div class="section-title">Teams Overview</div>
  <div class="team-grid">{cards_html}
  </div>

  <div class="section-title">Filter</div>
  <div class="search-box"><input type="text" id="searchInput" placeholder="🔍  Search issues and PRs..." oninput="applyFilters()"></div>
  <div class="filter-bar" id="teamFilters">{team_buttons}</div>

  <!-- TABS: Issues / PRs -->
  <div class="tab-bar">
    <div class="tab active" data-tab="issues" onclick="switchTab('issues')">🔴 Issues ({len(classified_issues)})</div>
    <div class="tab" data-tab="prs" onclick="switchTab('prs')">🟢 Pull Requests ({len(classified_prs)})</div>
  </div>

  <div class="tab-content active" id="tab-issues">
    <div class="counter" id="counter-issues"></div>
    <table>
      <thead><tr><th>#</th><th>Title</th><th>Team</th><th>Detected Path</th><th>Labels</th></tr></thead>
      <tbody id="issuesBody">{issue_rows}</tbody>
    </table>
  </div>

  <div class="tab-content" id="tab-prs">
    <div class="counter" id="counter-prs"></div>
    <table>
      <thead><tr><th>#</th><th>Title</th><th>Team</th><th>Detected Path</th><th>Labels</th></tr></thead>
      <tbody id="prsBody">{pr_rows}</tbody>
    </table>
  </div>

  <footer>
    Teams derived from <code>.github/CODEOWNERS</code> — each item assigned to one team by context matching<br>
    Generated on {now_str}
  </footer>
</div>

<script>
let activeTeam = 'all';
let activeTab = 'issues';

function switchTab(tab) {{
  activeTab = tab;
  document.querySelectorAll('.tab').forEach(t => t.classList.toggle('active', t.dataset.tab === tab));
  document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
  document.getElementById('tab-' + tab).classList.add('active');
  applyFilters();
}}

function filterByTeam(team) {{
  activeTeam = team;
  document.querySelectorAll('.filter-btn').forEach(b => {{
    b.classList.toggle('active', (team === 'all' && !b.dataset.team) || b.dataset.team === team);
  }});
  if (team === 'all') document.querySelector('.filter-btn:not([data-team])').classList.add('active');
  applyFilters();
}}

function applyFilters() {{
  const search = document.getElementById('searchInput').value.toLowerCase();
  ['issues', 'prs'].forEach(section => {{
    const bodyId = section === 'issues' ? 'issuesBody' : 'prsBody';
    const rows = document.querySelectorAll('#' + bodyId + ' .item-row');
    let count = 0;
    rows.forEach(row => {{
      const team = row.dataset.team;
      const text = row.textContent.toLowerCase();
      const teamMatch = activeTeam === 'all' || team === activeTeam;
      const searchMatch = !search || text.includes(search);
      const show = teamMatch && searchMatch;
      row.classList.toggle('hidden', !show);
      if (show) count++;
    }});
    document.getElementById('counter-' + section).textContent = count + ' items shown';
  }});
}}

applyFilters();
</script>
</body>
</html>"""

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(dashboard_html)
    print(f"✅ Dashboard written to {output_path}")


# ---------------------------------------------------------------------------
# 5. Main
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="Generate an Issues & PR dashboard grouped by CODEOWNERS teams."
    )
    parser.add_argument("--issues", default=str(SCRIPT_DIR / "_data" / "issues.json"),
                        help="Path to issues JSON file")
    parser.add_argument("--prs", default=str(SCRIPT_DIR / "_data" / "prs.json"),
                        help="Path to PRs JSON file")
    parser.add_argument("--codeowners", default=str(SCRIPT_DIR / ".." / ".." / ".github" / "CODEOWNERS"),
                        help="Path to CODEOWNERS file")
    parser.add_argument("--repo-root", default=str(SCRIPT_DIR / ".." / ".."),
                        help="Path to the repository root (for scanning subdirectories)")
    parser.add_argument("--extra-keywords", default=str(SCRIPT_DIR / "extra_keywords.json"),
                        help="Path to extra_keywords.json for manual keyword overrides")
    parser.add_argument("--output", default=str(SCRIPT_DIR / "output" / "dashboard.html"),
                        help="Output HTML path")
    parser.add_argument("--repo-url", default="https://github.com/microsoft/Windows-driver-samples",
                        help="Repository URL for links")
    args = parser.parse_args()

    # Parse CODEOWNERS
    print(f"📂 Parsing CODEOWNERS from {args.codeowners}")
    teams, sections, codeowners_rules = parse_codeowners(args.codeowners)
    repo_root = str(Path(args.repo_root).resolve())
    print(f"   Scanning repo tree at {repo_root}")
    team_keywords = build_keywords(
        teams, sections,
        repo_root=repo_root,
        extra_keywords_path=args.extra_keywords,
    )
    print(f"   Found {len(teams)} teams, {len(codeowners_rules)} CODEOWNERS rules")

    # Load data
    print(f"📥 Loading issues from {args.issues}")
    issues = load_issues(args.issues)
    print(f"   {len(issues)} open issues")

    print(f"📥 Loading PRs from {args.prs}")
    prs = load_prs(args.prs)
    print(f"   {len(prs)} open PRs")

    # Classify
    print("🏷️  Classifying items (one team per item)...")
    path_hits = 0
    classified_issues = []
    for i in issues:
        team, detected_path = classify_one(
            i["title"], i.get("body", ""), team_keywords, codeowners_rules
        )
        labels = [l["name"] for l in i.get("labels", [])]
        if detected_path:
            path_hits += 1
        classified_issues.append({
            "number": i["number"],
            "title": i["title"],
            "team": team,
            "labels": labels,
            "user": i["user"]["login"],
            "url": args.repo_url + "/issues/" + str(i["number"]),
            "detected_path": detected_path,
        })

    classified_prs = []
    for pr in prs:
        team, detected_path = classify_one(
            pr["title"], pr.get("body", ""), team_keywords, codeowners_rules
        )
        labels = [l["name"] for l in pr.get("labels", [])]
        if detected_path:
            path_hits += 1
        classified_prs.append({
            "number": pr["number"],
            "title": pr["title"],
            "team": team,
            "labels": labels,
            "user": pr["user"]["login"],
            "url": args.repo_url + "/pull/" + str(pr["number"]),
            "detected_path": detected_path,
        })

    print(f"   📁 {path_hits}/{len(classified_issues)+len(classified_prs)} items classified via detected file/folder path")

    all_teams = sorted(set(list(teams.keys())))

    # Print summary
    print("\n📊 Team breakdown:")
    for t in sorted(all_teams):
        ni = sum(1 for i in classified_issues if i["team"] == t)
        np_ = sum(1 for p in classified_prs if p["team"] == t)
        if ni + np_ > 0:
            print(f"   {t}: {ni} issues, {np_} PRs")

    # Generate
    print(f"\n📝 Generating dashboard...")
    generate_dashboard(classified_issues, classified_prs, all_teams, args.repo_url, args.output)


if __name__ == "__main__":
    main()
