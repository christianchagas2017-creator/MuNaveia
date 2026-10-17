EDITOR VISUAL DA HUD - MuVelox
================================

Arquivos integrados
-------------------
- source/NewUIHudEditor.cpp
- source/NewUIHudEditor.h
- source/NewUIMainFrameWindow.cpp (coordenadas e hitboxes ligadas ao editor)
- source/GlobalBitmap.cpp (limpeza de RGB em pixels transparentes da HUD)
- source/ZzzInterface.cpp (comando /hud)
- Main.vcxproj e Main.vcxproj.filters

Como usar no cliente
--------------------
1. Compile a configuracao do cliente com CLIENT_VERSION >= 1001.
2. Copie para a pasta Data do cliente as pastas
   Data\\Custom\\Interface\\HUD_MuAway e Data\\Custom\\Interface\\HUD_MuAway_V24
   incluidas neste pacote. A primeira e o pack original enviado pelo usuario e
   contem menu01/menu02/menu03, laterais, orbs, barras SD/AG, icones e skills.
   A segunda fornece somente a barra EXP compatível com esta source. A ferramenta cria
   Custom\\HUDLayout.ini na primeira vez que o botao SALVAR for usado.
   Nesta versao o `Custom\\HUDLayout.ini` ja vem preenchido com o layout
   enviado pelo usuario e usa ponto decimal (por exemplo `145.09`), pois a
   virgula ja e o separador dos campos.
3. Entre no mapa e digite /hud no chat.
4. Clique em um retangulo da HUD e arraste.
5. Clique em SALVAR (ou pressione F5/Ctrl+S).

Controles
---------
- /hud: abre/fecha o editor.
- SALVAR: grava Custom\\HUDLayout.ini e Custom\\HUDLayout.generated.h.
- RESET: volta ao layout 1920x1080 do header enviado; salve depois se quiser manter.
- CARREGAR: recarrega o arquivo INI.
- FECHAR ou Esc: fecha/deseleciona.
- MODO: HUD / tecla G: alterna entre mover a HUD inteira pelo FRAME ou mover
  somente o elemento selecionado. O comando /hud hud ativa HUD inteira e
  /hud item ativa somente item.
- Setas: move 1 unidade do PSD; Shift + seta move 10.
- Ctrl + seta: altera largura/altura; Ctrl + arraste pela quina inferior direita
  tambem redimensiona.
- Ctrl + T: aumenta largura e altura somente do item selecionado; Ctrl + Shift + T
  diminui as duas medidas.
- + / - ou roda do mouse: altera a escala do item selecionado.
- Alt + arraste no FRAME: move somente o FRAME, mesmo no modo HUD.
- Tab: seleciona o proximo elemento; Delete: alterna visibilidade.

Atualizacao em tempo real
--------------------------
- HP, MP, SD e AG sao calculados novamente durante cada quadro da HUD a partir
  dos valores atuais de CharacterAttribute; nao sao imagens estaticas.
- SD e AG usam o maximo valido de CharacterAttribute/Master_Level_Data e o valor
  atual nunca fica preso a um maximo zerado ou atrasado; o percentual e recalculado
  a cada quadro.
- A moldura e montada em cinco pecas do pack (`menu01_new_wide`, `menu01`,
  `menu02`, `menu03`, `menu03_new_wide`) dentro do retangulo FRAME. Isso evita
  esticar uma imagem unica e preserva as coordenadas do `HUDLayout.ini`.
- Para testar, tome dano ou gaste mana/AG. Uma potion usada com o valor ja cheio
  nao produz diferenca visual.
- O PSD de referencia 1920x1080 esta no pacote como
  HUD_MuVelox_Referencia.psd.

Elementos disponiveis
---------------------
FRAME, FPS, COORDS, HP_GAUGE, HP_NUMBER, MP_GAUGE, MP_NUMBER, AG_GAUGE,
AG_NUMBER, SD_GAUGE, SD_NUMBER, EXP_GAUGE, EXP_NUMBER, SKILL_CURRENT,
POTION_Q, POTION_W, POTION_E, BUTTON_PARTY, BUTTON_CHARACTER,
BUTTON_INVENTORY, BUTTON_RIGHT_TOP, BUTTON_RIGHT_BOTTOM, SKILL_ARROW_UP,
SKILL_ARROW_DOWN e SKILL_POPUP.

Observacoes importantes
-----------------------
- Arrastar FRAME move a HUD inteira, incluindo as camadas dinamicas, para manter
  frame, orbs, numeros, itens, botoes e skill sincronizados.
- O `Custom\\HUDLayout.ini` e a fonte principal dos valores de posicao, tamanho,
  escala e visibilidade. O codigo carrega esse arquivo na inicializacao, portanto
  alterar somente o INI nao exige recompilacao. O `HUDLayout.generated.h` e apenas
  a reserva compilada para o caso de o INI nao existir; ele nao deve ser editado
  para substituir o INI.
- Depois de alterar `source/*.cpp`, compile novamente o projeto `Vox` na
  configuracao `x86` (CLIENT_VERSION >= 1001) e substitua o executavel gerado.
  Alterar somente o INI nao exige recompilacao; alterar a SRC exige.
- O carregador da HUD zera a cor RGB dos pixels com alfa 0 antes de enviar a
  textura para OpenGL. Isso evita bordas escuras/serrilhadas quando a HUD usa
  filtro linear, sem mudar a arte visivel.
- Neste branch CLIENT_VERSION >= 1001 a source possui tres hotkeys de item
  (Q/W/E). O branch antigo e que declara a hotkey R.
- Nao renomeie os arquivos `.OZT`. O codigo chama o caminho logico `.tga` e o
  carregador troca automaticamente a extensao para abrir o `.OZT` correspondente.
- O codigo recorta o padding transparente do pack: orb 53x48 e SD/AG 15x36.
  Isso evita que a barra pareca cortada ou fique deslocada dentro da caixa.
- Os numeros HP/MP/SD/AG usam exatamente as escalas definidas no layout enviado e
  nao recebem mais um aumento automatico escondido em resolucoes menores.
- A quantidade dos itens Q/W/E e somada diretamente do inventario. Se a lista
  visual ainda estiver atualizando, o contador usa a durabilidade do item do
  atalho como fallback para nao desaparecer.
- Os botoes Party/Character/Inventory mantem o icone nativo com alpha correto;
  ao clicar, o icone desce 1 px e escurece levemente, retornando ao normal ao
  fechar a janela. Os quadros `btn_*_pressed` incompatíveis nao sao carregados.
- As alteracoes novas estao marcadas com //newhudvictoriuri.
