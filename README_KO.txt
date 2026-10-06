영걸전복각 유광판 4.0 - Nintendo Switch 포팅 작업본

현재 단계: Switch/libnx + SDL2 부트스트랩 프로젝트 + Android판 게임 데이터(romfs) 결합.
아직 플레이 가능한 NRO가 아닙니다. JYSDL 네이티브 API와 Lua 5.2 결합이 다음 단계입니다.

확인된 게임측 lib API:
Background, CharSet, Debug, Delay, DrawRect, DrawStr, EnableKeyRepeat, FillColor,
FreeSur, GetKey, GetMouse, GetTime, KoSaveIO, LoadConfig, LoadSoundConfig, LoadSur,
PicGetXY, PicLoadCache, PicLoadFile, PlayMIDI, PlayWAV, SaveSur, SetClip, ShowSlow, ShowSurface

빌드 요구사항:
- devkitPro/devkitA64 + libnx
- Switch portlibs: SDL2, SDL2_image, SDL2_ttf, SDL2_mixer
- Lua 5.2 호환 라이브러리(프로젝트에서 -llua로 연결되도록 설치/빌드)

현재 ChatGPT 작업 환경에는 devkitA64/libnx/elf2nro가 설치되어 있지 않아 NRO 바이너리를 생성하지 못했습니다.

[Phase 2]
Lua 런타임 및 JYSDL lib 호환층이 추가되었습니다.
필요 portlibs: SDL2, SDL2_image, SDL2_ttf, SDL2_mixer, Lua 5.2.
공식 switch-examples와 동일하게 DEVKITPRO/libnx switch_rules를 사용합니다.
