# NVim

NVim GUI For Mac OS X

<details open="open">
  <summary><h2 style="display: inline-block">Table of Contents</h2></summary>
  <ol>
    <li><a href="#getting-started">Getting Started</a></li>
    <li><a href="#acknowledgements">Acknowledgements</a></li>
    <li><a href="#license">License</a></li>
  </ol>
</details>

## Getting Started

### Requirements

- macOS >= v13.3
- NeoVim >= v0.9.02

### For developer

```bash
$ pod install
$ open NVim.xcworkspace
```

Delete quarantine attribute:
```bash
$ xattr -dr com.apple.quarantine ./Venders/NeoVim/nvim
```

## Acknowledgements

This application makes use of the following third party libraries:
- [CWPack](https://github.com/clwi/CWPack) - [`LICENSE`](Venders/CWPack/LICENSE)

## License

Distributed under the MIT License. See [`LICENSE`](LICENSE) for more information.
