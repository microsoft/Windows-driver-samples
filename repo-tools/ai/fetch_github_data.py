#!/usr/bin/env python3
"""
fetch_github_data.py — Fetches open issues and PRs from a GitHub repo using
the GitHub REST API and saves them as JSON for generate_dashboard.py.

Usage:
    python fetch_github_data.py [--repo OWNER/REPO] [--output-dir DIR]

Requires:
    - GITHUB_TOKEN environment variable (for authenticated API access)
    - OR works without auth for public repos (rate-limited to 60 req/hr)
"""

import argparse
import json
import os
import sys
import urllib.request
import urllib.error
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent


def fetch_paginated(url: str, token: str | None = None) -> list[dict]:
    """Fetch all pages from a GitHub REST API endpoint."""
    results = []
    page = 1
    while True:
        sep = "&" if "?" in url else "?"
        page_url = f"{url}{sep}page={page}&per_page=100&state=open"
        req = urllib.request.Request(page_url)
        req.add_header("Accept", "application/vnd.github+json")
        req.add_header("User-Agent", "repo-tools-dashboard")
        if token:
            req.add_header("Authorization", f"Bearer {token}")

        try:
            with urllib.request.urlopen(req) as resp:
                data = json.loads(resp.read().decode("utf-8"))
        except urllib.error.HTTPError as e:
            print(f"❌ HTTP {e.code} fetching {page_url}: {e.reason}")
            sys.exit(1)

        if not data:
            break
        results.extend(data)
        if len(data) < 100:
            break
        page += 1
        print(f"   Fetched page {page - 1} ({len(results)} items so far)")

    return results


def main():
    parser = argparse.ArgumentParser(description="Fetch GitHub issues and PRs as JSON.")
    parser.add_argument("--repo", default="microsoft/Windows-driver-samples",
                        help="GitHub repo in OWNER/REPO format")
    parser.add_argument("--output-dir", default=str(SCRIPT_DIR / "_data"),
                        help="Output directory for JSON files")
    args = parser.parse_args()

    token = os.environ.get("GITHUB_TOKEN")
    if not token:
        print("⚠️  GITHUB_TOKEN not set. Using unauthenticated access (60 req/hr limit).")

    os.makedirs(args.output_dir, exist_ok=True)
    base = f"https://api.github.com/repos/{args.repo}"

    # Fetch issues (the API returns PRs mixed in; filter them out)
    print(f"📥 Fetching open issues from {args.repo}...")
    all_items = fetch_paginated(f"{base}/issues", token)
    issues = [i for i in all_items if "pull_request" not in i]
    print(f"   ✅ {len(issues)} open issues")

    issues_path = os.path.join(args.output_dir, "issues.json")
    with open(issues_path, "w", encoding="utf-8") as f:
        json.dump({"issues": issues}, f, indent=2)
    print(f"   Saved to {issues_path}")

    # Fetch PRs
    print(f"📥 Fetching open PRs from {args.repo}...")
    prs = fetch_paginated(f"{base}/pulls", token)
    print(f"   ✅ {len(prs)} open PRs")

    prs_path = os.path.join(args.output_dir, "prs.json")
    with open(prs_path, "w", encoding="utf-8") as f:
        json.dump(prs, f, indent=2)
    print(f"   Saved to {prs_path}")

    print("\n✅ Data fetched. Run generate_dashboard.py to build the dashboard.")


if __name__ == "__main__":
    main()
