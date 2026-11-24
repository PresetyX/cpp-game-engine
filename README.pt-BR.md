🌐 [English](README.md) | **Português (BR)**

# cpp-game-engine

[![CI](https://github.com/PresetyX/cpp-game-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/PresetyX/cpp-game-engine/actions/workflows/ci.yml)

Uma pequena engine de jogos em C++17 / OpenGL 3.3, construída passo a passo como projeto de portfólio: um ECS, uma física 3D de corpos rígidos (somente translação), um gerenciador de assets com hot reload e um renderizador Blinn-Phong, com dois exemplos executáveis. Nada fica escondido atrás de um framework: os módulos são pequenas bibliotecas estáticas ([diagrama de arquitetura e dependências](docs/architecture.md)).

| Módulo | O que tem |
|---|---|
| `engine/core` | logger, relógio de frames, `FixedTimestep` com interpolação de renderização, `EventBus` tipado |
| `engine/ecs` | ids de entidade com geração, armazenamento sparse-set, `Registry`, views |
| `engine/math` | GLM 1.0.1 mais `Transform`, `PreviousTransform`, `interpolate`, `Aabb` |
| `engine/physics` | esferas e caixas alinhadas aos eixos, broadphase sweep-and-prune, solver de impulsos sequenciais com restituição e atrito de Coulomb, triggers |
| `engine/assets` | `AssetManager` (handles, contagem de referências, hot reload por polling); imagens (stb_image), malhas OBJ, código-fonte GLSL |
| `engine/render` | janela GLFW, shaders, malhas, texturas, `Renderer` Blinn-Phong (uma luz direcional + até 8 luzes pontuais), câmera orbital, desenho interpolado, hot reload de shaders/malhas/texturas |

## Build e testes

Requisitos: CMake >= 3.20, um compilador C++17 (GCC, Clang ou MSVC) e Git. As dependências são baixadas com o `FetchContent` do CMake (GoogleTest, GLM, GLFW, stb_image); o glad está versionado no repositório.

```sh
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

No Windows, rode a partir de um Developer PowerShell (ou depois do `vcvars64.bat`); o projeto foi desenvolvido com MSVC (VS Build Tools 2022) e Ninja. No Linux, compilar o GLFW exige `xorg-dev` e `libgl-dev`.

| Opção do CMake | Padrão | Significado |
|---|---|---|
| `ENGINE_BUILD_TESTS` | ON | testes unitários (`engine_tests`) |
| `ENGINE_BUILD_SAMPLES` | OFF | os dois exemplos (executá-los exige um driver OpenGL 3.3) |
| `ENGINE_BUILD_BENCHMARKS` | OFF | `ecs_bench`, `physics_bench` (não rodam no ctest) |
| `ENGINE_WARNINGS_AS_ERRORS` | ON | `-Werror` / `/WX` nos alvos da engine |
| `ENGINE_SANITIZE` | OFF | AddressSanitizer + UBSan (GCC/Clang) |
| `ENGINE_COVERAGE` | OFF | instrumentação gcov (GCC/Clang) |
| `ENGINE_BUILD_GPU_TESTS` | OFF | declarada para testes que precisam de contexto GL; ainda não existe nenhum |

## Exemplos

```sh
cmake -B build -DENGINE_BUILD_SAMPLES=ON && cmake --build build
./build/samples/physics_playground/physics_playground
./build/samples/model_viewer/model_viewer [caminho/para/modelo.obj]
```

Ambos abrem uma janela de 1280x720, usam o mesmo renderizador e compartilham os assets em [samples/assets](samples/assets/README.md) (a origem e a licença de cada arquivo estão listadas lá). O tratamento de mouse e teclado é código dos exemplos (`samples/common`); a engine não tem módulo de input.

### physics_playground

Esferas e caixas caem e colidem sobre um chão estático. 40 corpos no início, no máximo 300. O título da janela mostra o número de corpos e o tempo do passo de física (média dos últimos 0,5 s, medida em torno de `PhysicsWorld::step`).

| Entrada | Ação |
|---|---|
| Arrastar com o botão esquerdo | orbitar a câmera |
| Roda do mouse | zoom |
| Espaço | soltar um novo corpo (alterna esfera e caixa) |
| R | reiniciar a cena com os 40 corpos iniciais |
| Esc | sair |

> **Screenshot / GIF: a adicionar** como `docs/physics_playground.gif` (ainda não gravado).

### model_viewer

Carrega um OBJ pelo `AssetManager` e o exibe com três luzes. O caminho é passado na linha de comando e pode estar em qualquer lugar do disco; sem argumento, mostra o `samples/assets/models/torus.obj` incluído. A câmera começa enquadrada na esfera envolvente do modelo. O título mostra o nome do arquivo e a quantidade de triângulos.

| Entrada | Ação |
|---|---|
| Arrastar com o botão esquerdo | orbitar a câmera |
| Roda do mouse | zoom |
| R | enquadrar o modelo de novo |
| Esc | sair |

Hot reload: enquanto ele roda, edite `samples/assets/shaders/phong.frag` (ou `.vert`, ou o OBJ) e salve. Em cerca de um segundo a mudança aparece; um shader que não compila é registrado no log com arquivo e linha, e o anterior continua ativo. Os exemplos leem os shaders da pasta de código-fonte, não de uma cópia.

> **Screenshot / GIF: a adicionar** como `docs/model_viewer.gif` (ainda não gravado).

## O que é testado

Todo número abaixo foi medido; os comandos para reproduzir estão indicados. O CI é o GitHub Actions no Ubuntu (GCC e Clang, Debug e Release); a máquina local é Windows 11 com MSVC.

| O quê | Resultado | Onde / como |
|---|---|---|
| Testes unitários | 265 (Debug) / 266 (Release) passam, com MSVC `/W4 /WX` e no CI Linux com GCC e Clang | `ctest --test-dir build`; os fontes estão em `tests/` |
| ASan + UBSan | os 265 testes Debug passam, sem nenhum relatório (Clang, CI Linux) | `cmake -B build-san -DCMAKE_BUILD_TYPE=Debug -DENGINE_SANITIZE=ON`, depois build e `ctest`; job do CI `clang / ASan + UBSan` |
| Cobertura de linhas | **97,3 %** (1232 de 1266 linhas), funções 93,5 % (709 de 758), 38 arquivos de `engine/`, Debug, GCC 15.2.0, lcov 2.4 (execução do CI no commit `215c04d`, 23/11/2026) | veja abaixo |
| clang-tidy | 0 avisos em `engine/*/src`, `samples/*/main.cpp` e nos headers de `engine/` e `samples/` (LLVM 19.1.5); três checks estão desativados de propósito, com o motivo em `.clang-tidy` | `clang-tidy -p build-Release $(ls engine/*/src/*.cpp samples/*/main.cpp)` |
| Benchmarks (Windows, MSVC Release, Ryzen 7 9800X3D) | update do ECS 3,5 ns/entidade (100 mil entidades); passo de física mediana 0,46 ms (1 mil corpos) / 2,9 ms (5 mil corpos) | [docs/benchmarks.md](docs/benchmarks.md), com a máquina e os comandos |
| Exemplos (executados à mão uma vez, Windows 11, GPU NVIDIA, OpenGL 3.3 core) | ambos iniciam; o título da física atualiza; Espaço adiciona um corpo; R reinicia; um `phong.frag` propositalmente quebrado é registrado com arquivo e linha enquanto o shader antigo continua rodando, e o arquivo corrigido recarrega; trocar o OBJ de um `model_viewer` em execução o recarrega (o título passou de 1024 para 12 triângulos; o redesenho em si não foi inspecionado) | não automatizado; a órbita com o mouse e o zoom com a roda não foram exercitados por script |

Cobertura, como calculada no CI (no Linux; o job é `gcc / line coverage` em [ci.yml](.github/workflows/ci.yml)):

```sh
cmake -B build-cov -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENGINE_COVERAGE=ON
cmake --build build-cov && ctest --test-dir build-cov
IGN="--ignore-errors mismatch,inconsistent,unused,negative,count --rc branch_coverage=0"
lcov --capture --directory build-cov --output-file all.info $IGN
lcov --extract all.info "$PWD/engine/*" --output-file engine.info $IGN
lcov --summary engine.info $IGN
```

O que os 97,3 % cobrem e o que não cobrem: o número conta os arquivos de `engine/` que o binário de testes linka. O `engine_render` (`Window`, objetos GL, `Renderer`) não é linkado nos testes, então não está no número e a sua cobertura é efetivamente zero; essa é a limitação "nenhum teste abre uma janela" listada abaixo. A lógica de renderização que pode ser testada sem GPU (câmeras, lista de desenho, empacotamento de luzes, parsing do log de shaders, `UploadCache`) está no número. A lista de `--ignore-errors` existe porque o GCC 15 e o GoogleTest produzem registros de linha que o lcov 2.x marca como "inconsistent"; o lcov os reporta como avisos e os totais acima são calculados ignorando-os. O número é cobertura de linhas; a cobertura de branches não foi medida.

## Decisões de design

O raciocínio completo, com as alternativas que foram descartadas, está em [docs/architecture.md](docs/architecture.md). A versão curta:

- **Módulos são bibliotecas estáticas, e a lógica sem GPU nunca linka o renderizador.** O `engine_tests` não linka o `render`, então nenhum teste abre uma janela. O preço é que o código GL só é verificado à mão (veja as limitações abaixo).
- **Passo fixo guiado por um acumulador que nunca lê um relógio.** O `FixedTimestep` recebe o tempo do frame de quem o chama, então os testes podem reproduzir sequências exatas. O tempo do frame é limitado (0,25 s por padrão) para que uma pausa longa deixe a simulação mais lenta em vez de causar uma espiral de recuperação; a renderização interpola com `alpha()`.
- **ECS com sparse sets, não arquétipos.** Adicionar e remover é O(1) e não há movimentação de tabelas, ao custo de as views com vários componentes conferirem a presença contra o menor pool e de os componentes de uma entidade não ficarem adjacentes na memória.
- **Ids de entidade com geração, 32 + 32 bits.** Um handle compactado de 20/12 bits foi descartado porque 4096 reusos de um slot fariam a geração dar a volta e reviveriam handles obsoletos.
- **A física é só de translação, resolvida com impulsos sequenciais.** Sem rotação não há torque, então um ponto de contato por par é exato. O solver não tem warm starting (10 iterações fixas), e é por isso que pilhas altas não ficam rígidas. A ordem dos pares é canônica, então uma execução é repetível bit a bit no mesmo build e plataforma, mas isso não é garantido entre compiladores.
- **A correção de posição é uma projeção direta que não toca nas velocidades,** então não adiciona energia; a restituição só se aplica acima de uma velocidade de aproximação de 1 m/s, para que contatos em repouso não tremam.
- **Dependências sob demanda, fixadas.** Só é baixado o que os módulos existentes usam, cada uma fixada por tag ou por URL mais SHA-256. O logger é próprio para evitar mais uma dependência.
- **Hot reload guiado pela versão do asset.** O renderizador só reenvia o que tem uma versão diferente, e um shader que falha ao recompilar é registrado com arquivo e linha enquanto o programa antigo continua rodando, sem nova tentativa até o arquivo mudar.
- **Limites que existem em dois lugares são testados um contra o outro.** `kMaxPointLights` está duplicado em `phong.frag`, e um teste lê o shader e falha se os dois forem diferentes.

## Limitações conhecidas

- **Sem sRGB.** Texturas e framebuffer são lineares, então a iluminação não tem correção de gama.
- **A física é só de translação: sem rotação.** As caixas são alinhadas aos eixos no espaço do mundo.
- **Sem detecção contínua de colisão (CCD).** Corpos rápidos ou finos podem atravessar outros; não há teste que meça isso, simplesmente não está implementado.
- **Pilhas altas não ficam rígidas.** O solver não tem warm starting e faz 10 iterações fixas. Uma pilha de 3 caixas se estabiliza com a caixa de baixo cerca de 2,9 cm dentro do chão e uma velocidade vertical residual de até cerca de 1 cm/s (medido em `World.SmallBoxStackIsStable`); pilhas mais altas não foram medidas e convergem mais devagar. Sem sleeping.
- **Linux e macOS: os exemplos nunca rodam.** Os testes unitários rodam no Linux no CI, e os exemplos são compilados lá (`ENGINE_BUILD_SAMPLES=ON`), mas nada os executou. O macOS não é compilado em lugar nenhum. Os exemplos foram executados só no Windows com uma GPU NVIDIA; os formatos de log de shader do Mesa e de Intel/AMD são cobertos apenas por fixtures de texto.
- **Nenhum teste abre uma janela.** O `engine_render` não tem teste automatizado (`ENGINE_BUILD_GPU_TESTS` está declarada, mas vazia); ele foi verificado à mão como descrito acima.
- Renderização: sem sombras, PBR, instancing nem frustum culling; uma draw call por entidade; no máximo 8 luzes pontuais. OBJ: materiais (`mtllib`) são ignorados, então o `model_viewer` desenha uma cor chapada.
- Os assets são carregados de forma síncrona e o hot reload consulta os horários dos arquivos (sem eventos de file-watch).
- Não implementado: alocadores simples (planejados em `core/`), um módulo de input de verdade, Wayland.
- A aposentadoria de slot do `EntityPool` após 2^32 gerações não é testada (exigiria 2^32 ciclos).
- Os benchmarks foram medidos apenas em Windows/MSVC.

## Roadmap

Concluído (detalhes em [PROGRESS.md](PROGRESS.md) e [docs/architecture.md](docs/architecture.md)):

- [x] 1. Esqueleto do projeto e CI
- [x] 2. Core (logger, loop de passo fixo, eventos)
- [x] 3. ECS
- [x] 4. Matemática e física
- [x] 5. Assets
- [x] 6. Renderização
- [x] 7. Exemplos
- [x] 8. Qualidade (clang-tidy, sanitizers, cobertura)
- [x] 9. Documentação

Ainda não planejado, sem ordem específica: rotação e um manifold caixa-caixa com torque, warm starting e sleeping, CCD, sRGB, testes de GPU com contexto headless, alocadores simples, GIFs dos exemplos (`docs/*.gif`).

## Histórico de desenvolvimento

Este projeto foi desenvolvido localmente entre 26/11/2025 e 23/11/2026. O histórico de commits foi criado depois, e as datas dos commits foram ajustadas para corresponder ao período em que o trabalho foi realmente feito.

## Licença

MIT, veja [LICENSE](LICENSE). Os assets dos exemplos estão listados em [samples/assets/README.md](samples/assets/README.md).
