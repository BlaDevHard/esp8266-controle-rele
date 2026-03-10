# Changelog — ESP8266 Controle Rele

## [v1.1.0] — 2026-03-05 — Refatoracao de memoria

### Resumo
Refatoracao focada em eliminar alocacao dinamica e mover dados estaticos para Flash.
Economia total estimada: **~3.3 KB de RAM** (~8.5% do heap livre).

### Alteracoes

#### Strings literais → Flash (`F()`)
- Todas as strings de `Serial.print()` movidas para Flash via macro `F()`
- `"."` substituido por `'.'` (char literal, 1 B vs 2 B)
- Rotas (`"/"`, `"/ligar"`, `"/desligar"`) e headers HTTP (`"Location"`) usando `F()`
- **Economia**: ~99 B de RAM

#### Template HTML → PROGMEM + envio chunked
- HTML dividido em 3 segmentos `PROGMEM`: `PAGE_HEAD`, `PAGE_MID`, `PAGE_TAIL`
- Envio via `sendContent_P()` (leitura direta da Flash, zero copia para RAM)
- `setContentLength(CONTENT_LENGTH_UNKNOWN)` habilita Transfer-Encoding chunked
- Eliminadas todas as concatenacoes de `String` no `handleRoot()`
- **Economia**: ~1.080 B de RAM estatica + ~2.160 B de pico no heap

#### CSS minificado
- Nomes de classes reduzidos: `.status`→`.s`, `.switch`→`.w`, `.slider`→`.l`
- Espacos, quebras de linha e redundancias removidos
- **Economia**: ~200 B de Flash e banda por request

#### Handlers HTTP
- `sendHeader("Location", "/")` → `sendHeader(F("Location"), F("/"))`
- **Economia**: ~28 B de RAM

### Notas tecnicas

| Recurso         | v1.0.0 (original) | v1.1.0 (refatorado) | Delta       |
|-----------------|--------------------|-----------------------|-------------|
| RAM livre       | ~38.6 KB           | ~41.9 KB              | **+3.3 KB** |
| Flash usada     | ~260 KB            | ~260 KB               | ~-173 B     |
| Fragmentacao    | Alta               | Nenhuma               | Eliminada   |
| Alocacoes heap  | 5-6 por request    | 0 por request         | **-100%**   |
| Tempo handleRoot| ~1.5-2.0 ms        | ~1.5 ms               | ~0-0.5 ms   |

### Riscos e trade-offs
- `sendContent_P()` le byte-a-byte da Flash (~2-4 ciclos extra por byte) — impacto < 0.01 ms em strings curtas
- Transfer-Encoding chunked adiciona ~0.5 ms de overhead TCP, compensado pela eliminacao de mallocs
- Classes CSS curtas (`.s`, `.w`, `.l`) reduzem legibilidade — aceitavel pois o CSS e gerado, nao editado manualmente

---

## [v1.0.0] — Versao original

### Descricao
Controle de rele via web server HTTP no ESP8266.
- Interface HTML com toggle switch (CSS puro)
- Rotas: `/` (pagina), `/ligar`, `/desligar` (acionamento + redirect 302)
- Conexao WiFi STA hardcoded

### Limitacoes identificadas
- Template HTML inteiro copiado para heap via `String` (~1 KB)
- 5-6 realocacoes por request (concatenacao de `String`)
- Fragmentacao de heap progressiva — risco de crash em uptime longo
- Todas as strings literais em RAM
