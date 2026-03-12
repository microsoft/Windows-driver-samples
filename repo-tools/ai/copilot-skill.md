---
name: "Generate Issues & PR Dashboard"
description: |
  Generates an HTML dashboard showing all open GitHub Issues and Pull Requests
  for the Windows-driver-samples repository, grouped by the owning team from
  .github/CODEOWNERS. Each item is assigned to exactly one team based on a
  multi-layer context matching strategy: CODEOWNERS path keywords, repo
  subdirectory scanning, section comment names, manual overrides
  (extra_keywords.json), normalized matching (strips separators), and
  word-boundary matching for short keywords.
steps:
  - name: "Fetch GitHub data"
    description: "Download open issues and PRs from GitHub REST API"
    command: "python repo-tools/ai/fetch_github_data.py"
    notes: |
      - Set GITHUB_TOKEN env var for authenticated access (higher rate limit).
      - Saves JSON files to repo-tools/ai/_data/.
      - Use --repo flag to target a different repository.

  - name: "Generate dashboard"
    description: "Build the HTML dashboard from fetched data and CODEOWNERS"
    command: "python repo-tools/ai/generate_dashboard.py"
    notes: |
      - Parses .github/CODEOWNERS automatically for team definitions.
      - HIGHEST PRIORITY: Extracts file/folder paths from issue/PR text and
        matches them against CODEOWNERS rules (same last-match-wins as GitHub).
        Shows "Detected Path" column in the dashboard.
      - Falls back to keyword matching: path keywords, subdir scanning,
        section comments, extra_keywords.json, normalized + word-boundary matching.
      - Each issue/PR is assigned to exactly ONE team (most specific match).
      - Output goes to repo-tools/ai/output/dashboard.html.

  - name: "Open dashboard"
    description: "Open the generated HTML file in a browser"
    command: "start repo-tools/ai/output/dashboard.html"
    platform: "windows"

  - name: "Customize team keywords (optional)"
    description: "Edit extra_keywords.json to add domain-specific terms"
    file: "repo-tools/ai/extra_keywords.json"
    notes: |
      - Keys are CODEOWNERS team aliases (e.g. "display-kernel-devs").
      - Values are lists of additional keyword strings.
      - Use this when issues mention technology names not in CODEOWNERS paths.

output:
  path: "repo-tools/ai/output/dashboard.html"
  type: "html"
  description: "Self-contained HTML dashboard with Issues tab, PRs tab, team filters, and search."

parameters:
  - name: "repo"
    description: "Target GitHub repository in OWNER/REPO format"
    default: "microsoft/Windows-driver-samples"
    required: false
  - name: "github_token"
    description: "GitHub personal access token for API authentication"
    env: "GITHUB_TOKEN"
    required: false
---
