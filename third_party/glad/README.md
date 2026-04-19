# glad (generated, committed)

OpenGL 3.3 core loader, no extensions. Committed so that neither CI nor a normal build needs
Python or network access for it.

- Generator: glad2 **2.0.8** (`pip install glad2==2.0.8`)
- Command, run from the repository root (`noext.txt` is an empty file):

```sh
python -m glad --api gl:core=3.3 --extensions noext.txt --out-path third_party/glad c --loader
```

- Usage: `gladLoadGL(glfwGetProcAddress)` after `glfwMakeContextCurrent`.
- Licence: generated code is WTFPL OR CC0-1.0; `KHR/khrplatform.h` is Apache-2.0 (header comments).

SHA-256 of the committed files (`.gitattributes` marks them `-text` so they are never converted):

```
6f9452e24f48e68a48305118d52ff81e1da3ccd9e0f2e264ea38597e24969430  include/glad/gl.h
7b1e01aaa7ad8f6fc34b5c7bdf79ebf5189bb09e2c4d2e79fc5d350623d11e83  include/KHR/khrplatform.h
f89c7528f44bfa7d8f1fa64ddc22b5a37ba0341f0bfeca973e30d30dc630a7c4  src/gl.c
```
