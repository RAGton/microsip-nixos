# MicroSIP — Native Linux / NixOS Port (Qt 6 + PJSIP)

> **Aviso Legal / Attribution:** Este projeto é um port nativo comunitário para **Linux / NixOS** baseado no código-fonte aberto do [**MicroSIP**](https://www.microsip.org/) (v3.23.5). Todos os direitos sobre o projeto original, marca e design pertencem ao **MicroSIP (https://www.microsip.org/)**. Distribuído sob a licença **GNU General Public License v2.0 (GPL-2.0-or-later)**.

---

## Sobre o Projeto

O **MicroSIP** original é um softphone SIP leve e confiável para Windows baseado em **PJSIP** e **MFC**.  
Este repositório traz um **port 100% nativo para Linux e NixOS** que substitui a camada Windows/MFC por **Qt 6 (C++20)** mantendo a engine **PJSIP**, o schema **SQLite** e a estrutura de configuração compatível com `microsip.ini`.

### Recursos Implementados
- **Motor SIP/Media nativo (PJSIP 2.17)**: UDP, TCP, TLS, SRTP (com indicador `🔒 SRTP`), ICE, STUN, codecs de áudio configuráveis (Opus, G.722, PCMU, PCMA, GSM, etc.), VAD e Echo Cancellation (EC).
- **Múltiplas contas SIP**: troca rápida no topo da janela, edição/criação em 1 clique e auto-reconexão automática após queda de rede ou suspend/resume.
- **Chamadas completas**: múltiplas chamadas simultâneas, Hold, Mute, DTMF (Auto / RFC 2833 / SIP INFO + feedback sonoro local), Transferência Cega e Assistida, Conferência e Encaminhamento (`302`).
- **Gravação de chamadas (WAV)**: manual ou automática em `~/.local/share/microsip/recordings/`.
- **DND, Auto-Answer (AA) e Call Forwarding (FWD)** com pills de acesso rápido na barra de status e na bandeja do sistema.
- **Atalhos / BLF (Busy Lamp Field)**: painel lateral ou inferior com monitoramento de presença em tempo real e **Call Pickup** automático.
- **Mensagens SIP (SIMPLE IM)** com histórico salvo em SQLite.
- **Contatos e Histórico de Chamadas (SQLite)** com busca, favoritos, presença BLF e **Importar / Exportar (CSV e `Contacts.xml` do MicroSIP)**.
- **Integração nativa Linux**: Instância única via socket UNIX (`sip:`, `tel:`, `callto:`), System Tray, notificações desktop, temas **Escuro e Claro**, e proteção `0600` em `~/.config/microsip/microsip.ini`.

---

## Como Executar (Nix Flakes)

```bash
# Compilar e executar diretamente:
nix run .

# Apenas compilar:
nix build
./result/bin/microsip-qt
```

### Opções de Linha de Comando (CLI)
```bash
microsip-qt --minimized      # Inicia minimizado na bandeja do sistema
microsip-qt sip:100@servidor # Disca usando a instância já em execução
microsip-qt --answer         # Atende a chamada recebida ativa
microsip-qt --hangupall      # Encerra todas as chamadas ativas
microsip-qt --exit           # Fecha a instância em execução
```

---

## Instalação no NixOS / Home Manager

Adicione ao seu `flake.nix`:

```nix
inputs.microsip-qt.url = "github:RAGton/microsip-nixos";
```

### NixOS (`configuration.nix`)
```nix
imports = [ inputs.microsip-qt.nixosModules.default ];

programs.microsip-qt.enable = true;
```

### Home Manager (`home.nix`)
```nix
imports = [ inputs.microsip-qt.homeManagerModules.default ];

programs.microsip-qt = {
  enable = true;
  autostart = true; # inicia minimizado via systemd --user após o PipeWire
};
```

---

## Direitos Autorais e Licença

- **Projeto Original:** MicroSIP — [https://www.microsip.org/](https://www.microsip.org/)
  - `Copyright (C) 2011-2026 MicroSIP (https://www.microsip.org)`
- **Port Nativo Linux / NixOS (Qt 6):**
  - `Copyright (C) 2026 Gabriel Aguiar Rocha (RAGton) <aguiarrocha37@outlook.com>`
- **PJSIP Stack:** [https://www.pjsip.org/](https://www.pjsip.org/)
- **Licença:** [GNU General Public License v2.0 (GPL-2.0-or-later)](LICENSE)

Este software é software livre; você pode redistribuí-lo e/ou modificá-lo sob os termos da **GNU General Public License versão 2** (ou posterior), conforme publicada pela Free Software Foundation. Consulte o arquivo [LICENSE](LICENSE) para detalhes completos.
