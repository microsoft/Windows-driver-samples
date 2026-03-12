# Issues & PR Dashboard Generator

> **Location:** `repo-tools/ai/`
> A reusable tool that generates an HTML dashboard of all open GitHub Issues and
> Pull Requests, automatically grouped by the owning team from `.github/CODEOWNERS`.

## Quick Start

```bash
# 1. Fetch latest data from GitHub
python repo-tools/ai/fetch_github_data.py

# 2. Generate the dashboard
python repo-tools/ai/generate_dashboard.py

# 3. Open it
start repo-tools/ai/output/dashboard.html   # Windows
open  repo-tools/ai/output/dashboard.html   # macOS
```

## How It Works

1. **`fetch_github_data.py`** — Calls the GitHub REST API to download all open
   issues and PRs into `repo-tools/ai/_data/` as JSON files.
2. **`generate_dashboard.py`** — Parses `.github/CODEOWNERS` to build
   team → path mappings, classifies **each item to exactly one team** by
   matching path references and keywords in the title/body, and outputs a
   self-contained HTML dashboard.

### Team Assignment Rules

Each issue or PR is assigned to **exactly one** CODEOWNERS team using a
multi-layer matching strategy (highest priority first):

1. **🏆 Path detection + CODEOWNERS matching** *(highest priority)* — the tool
   extracts file/folder paths mentioned in the issue/PR text (GitHub URLs,
   file names with extensions, `.github/` references, known directory paths)
   and matches them against CODEOWNERS rules using the same last-match-wins
   logic GitHub uses. This is the most reliable signal.
2. **CODEOWNERS path keywords** — path segments from each entry
   (e.g., `network/trans` → team `netsec`).
3. **Repo subdirectory scanning** — subdirectory names under owned paths
   (e.g., `KMDOD` under `/video/` → team `display-kernel-devs`).
4. **Section comment names** — the `# Comment` lines above CODEOWNERS entries
   are used as fallback context keywords.
5. **Manual overrides** — `extra_keywords.json` provides domain-specific terms
   not derivable from paths (e.g., `wddm` → `display-kernel-devs`).
6. **Normalized matching** — strips whitespace/separators to catch variants
   like "WMI ACPI" matching path keyword `wmiacpi`.
7. **Word-boundary matching** — short keywords (< 4 chars) use `\bkw\b` regex
   to prevent false positives like "pos" matching "posted".

The **longest (most specific) match wins** at each layer. Items that don't
match any specific team fall back to `driver-samples-maintainers`.

### Dashboard Features

- **Two separate sections**: Issues tab and Pull Requests tab
- **Team overview cards** with issue/PR counts per team
- **Detected Path column** — shows the file/folder that triggered the team assignment
- **Filter by team** — click a card or use the filter buttons
- **Search** — free-text search across all items
- **Direct GitHub links** — every item links to its GitHub page

## Files

| File | Purpose |
|------|---------|
| `fetch_github_data.py` | Downloads issues/PRs from GitHub API |
| `generate_dashboard.py` | Classifies items and generates HTML dashboard |
| `extra_keywords.json` | Manual keyword overrides per team (editable) |
| `copilot-skill.md` | Skill definition for AI agent invocation |
| `_data/` | Downloaded JSON data (gitignored) |
| `output/` | Generated HTML dashboard (gitignored) |

## CLI Options

### fetch_github_data.py

| Flag | Default | Description |
|------|---------|-------------|
| `--repo` | `microsoft/Windows-driver-samples` | GitHub repo (`OWNER/REPO`) |
| `--output-dir` | `repo-tools/ai/_data` | Where to save JSON files |

Set `GITHUB_TOKEN` env var for authenticated access (recommended).

### generate_dashboard.py

| Flag | Default | Description |
|------|---------|-------------|
| `--issues` | `_data/issues.json` | Path to issues JSON |
| `--prs` | `_data/prs.json` | Path to PRs JSON |
| `--codeowners` | `../../.github/CODEOWNERS` | Path to CODEOWNERS |
| `--repo-root` | `../..` | Repo root for subdirectory scanning |
| `--extra-keywords` | `extra_keywords.json` | Manual keyword overrides |
| `--output` | `output/dashboard.html` | Output HTML path |
| `--repo-url` | `https://github.com/microsoft/Windows-driver-samples` | Repo URL for links |

## Customizing Team Keywords

Edit `extra_keywords.json` to add domain-specific terms:

```json
{
    "display-kernel-devs": ["wddm", "kmdod", "display driver"],
    "filesystems": ["fat"],
    "netsec": ["wfp", "wfpsampler"]
}
```

Keys are CODEOWNERS team aliases (without `@microsoft/` prefix).

## Using with AI Agents (Copilot Skill)

This tool is designed to be invoked by AI agents. The recommended workflow:

```
1. Run: python repo-tools/ai/fetch_github_data.py
2. Run: python repo-tools/ai/generate_dashboard.py
3. The dashboard is at: repo-tools/ai/output/dashboard.html
```

See `copilot-skill.md` for the structured skill definition.
