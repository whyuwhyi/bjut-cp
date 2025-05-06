# compiler

A modular compiler frontend project developed for coursework and experimentation,
featuring a configurable lexer and parser framework. This project supports multiple
parsing strategies including recursive descent and various LR-based algorithms.

---

## ✨ Features

- Modular lexer and parser architecture
- Support for:
  - Recursive Descent Parser
  - LR(0), SLR(1), and LR(1) Parsers
- Abstract Syntax Tree and Syntax Tree generation
- Configurable build via `Kconfig` (inspired by Linux kernel)
- Platform-independent `Makefile` build system

---

## 📁 Directory Layout

.
├── Kconfig # Configuration options (Kconfig system)
├── Makefile # Cross-platform Makefile
├── include/ # Public header interfaces
│ ├── common.h
│ ├── generated/ # Auto-generated headers (e.g., autoconf.h)
│ ├── lexer-analyzer/ # Lexer headers
│ ├── parser/ # Parser public interfaces (AST, API)
│ └── utils.h
├── src/ # Implementation files
│ ├── lexer-analyzer/ # Lexer logic
│ ├── parser/ # Parser logic
│ │ ├── lr/ # LR parser framework
│ │ ├── recursive_descent/
│ │ └── grammar.c/h, parser_common.c/h, etc.
│ ├── utils/ # Utility helpers
│ ├── lexer-main.c # Lexer CLI driver
│ └── parser-main.c # Parser CLI driver
└── scripts/ # Build system integration (kconfig.mk)

---

## ⚙️ Configuration (Kconfig System)

Before building, run:

```bash
make menuconfig

This will bring up a text-based configuration interface similar to the Linux kernel. You can select:

    Whether to enable debug output

    Which parser algorithm to use:

        Recursive Descent

        LR (with subtypes LR(0), SLR(1), LR(1))

    Whether to output the syntax tree

    Whether to print leftmost derivation

Once configured, header file include/generated/autoconf.h will be created.
🛠️ Building

To build the entire project:

make

To clean all generated files:

make clean

🖥️ Platform Compatibility

The Makefile detects your operating system and automatically adapts:

    On Linux/macOS: uses mkdir -p and rm -rf

    On Windows: uses if not exist and del /Q

🧪 Testing

Unit tests are implemented in src/unittest/. Build artifacts and object files are placed under build/.
📜 Origin of Configuration System

This project uses a lightweight Kconfig system ported from the open-source project 一生一芯 (Yisheng Yixin), which itself derives from the GNU/Linux kernel configuration infrastructure. It is a robust and scalable solution suitable for embedded toolchains, compiler frameworks, and large-scale configurable C projects.
📄 License

This project is released as part of coursework at BJUT. Licensing is TBD. Contact the author for reuse or redistribution.
