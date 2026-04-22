# DungeonForged

Jogo de ação / ARPG de terceira pessoa, estilo *dungeon crawler* roguelike, com **data-driven design** (DataTables, GAS) em **Unreal Engine 5.4**.

## Requisitos

- Unreal Engine **5.4** (via Epic Games Launcher)
- **Visual Studio 2022** (workload C++ game development) no Windows

## Estrutura

- `Source/DungeonForged/` — módulo C++ (GAS, combate, UI, `Run/DFRunManager`, `Run/DFSaveGame`, etc.)
- `Config/` — configuração do projeto
- `Content/` — conteúdo (parcialmente ignorado no Git por tamanho / assets de marketplace; ajuste `.gitignore` se quiser versionar tudo)

## Abrir o projeto

1. Clique com o botão direito em `DungeonForged.uproject` → **Generate Visual Studio project files** (se necessário).
2. Abra o `.sln` ou o `.uproject` no Unreal.

## Controle de versão

Clone o repositório, depois:

```text
cd DungeonForged
```

*(Opcional)* Regenerar arquivos do Visual Studio a partir do `.uproject`.

## Licença

Defina a licença do projeto (por exemplo adicionar um ficheiro `LICENSE`).
