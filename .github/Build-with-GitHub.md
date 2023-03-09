# Using GitHub actions for building drivers

If you use GitHub to host your code, you can leverage [GitHub Actions](https://docs.github.com/en/actions) to create automated workflows to build your driver projects.

`windows-2022` runner (provided by `windows-latest`) is configured with Windows Driver Kit version 22H2 and Visual Studio 2022 off the box, so most solutions can be built directly running `msbuild` directly.

```yaml
name: Build all driver samples
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
    runs-on: windows-2022
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
