# Using GitHub actions for building drivers

If you use GitHub to host your code, you can leverage [GitHub Actions](https://docs.github.com/en/actions) to create automated workflows to build your driver projects.

`windows-2025-vs2026` runner is configured with Visual Studio 2026 off the box, so most solutions can be built by running `msbuild` directly using the WDK NuGet package.

```yaml
name: Build driver solution
on:
  push:
    branches:
      - main
jobs:
  build:
    strategy:
      matrix:
        configuration: [Debug, Release]
        platform: [x64]
    runs-on: windows-2025-vs2026
    env:
      Solution_Path: path\to\driver\solution.sln
    steps:
      - name: Check out repository code
        uses: actions/checkout@v3

      - name: Add MSBuild to PATH
        uses: microsoft/setup-msbuild@v1.0.2

      - name: Build solution
        run: |
          msbuild ${{ env.Solution_Path }} -p:Configuration:${{ env.Configuration }} -p:Platform:${{ env.Platform }}
        env:
          Configuration: ${{ matrix.configuration }}
          Platform: ${{ matrix.platform }}
```
