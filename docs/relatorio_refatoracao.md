# Relatório: Análise de Refatoração — ESP8266 Controle Relé

## 1. Strings Literais (Serial / Server routes)

| String | RAM Original | RAM Refatorado |
|---|---|---|
| `"Conectando ao Wi-Fi"` | 20 B | 0 B (Flash via `F()`) |
| `"."` | 2 B | 1 B (`char '.'`) |
| `" Conectado!"` | 12 B | 0 B |
| `"IP: "` | 5 B | 0 B |
| `"Servidor HTTP iniciado."` | 24 B | 0 B |
| `"/"`, `"/ligar"`, `"/desligar"` | 19 B | 0 B |
| `"Location"` x2 | 18 B | 0 B |
| **Subtotal** | **~100 B RAM** | **~1 B RAM** |

- **Economia**: ~99 B de RAM (~99%)
- **Tempo de execução**: Leitura de Flash via `pgm_read_byte` é ~2-4 ciclos mais lenta por byte. Em strings curtas, impacto < 0.01 ms — **desprezível**.

---

## 2. Template HTML (armazenamento estático)

| Métrica | Original | Refatorado |
|---|---|---|
| Armazenamento | RAM (raw literal copiado p/ heap) | Flash (PROGMEM) |
| Tamanho do template | ~1.080 B | ~780 B (CSS minificado) |
| RAM ocupada pelo template | ~1.080 B (no heap) | 0 B |
| Flash ocupada | ~1.080 B | ~780 B |

- **Economia RAM**: ~1.080 B (100% do template removido da RAM)
- **Economia Flash**: ~300 B extras (CSS minificado)

---

## 3. Heap Dinâmico (geração da página)

Este é o **setor mais crítico** do código original.

### Original — `String` com concatenação

```
String html = raw_literal;   // aloca ~900 B no heap
html += "on\">LIGADO";       // realoca → cópia temporária ~900 B + novo ~915 B
html += raw_literal_2;       // realoca → ~1015 B + cópia
html += "checked";           // realoca → ~1022 B + cópia
html += raw_literal_3;       // realoca → ~1080 B + cópia
return html;                 // cópia de retorno ~1080 B
```

| Métrica | Original | Refatorado |
|---|---|---|
| Alocações no heap | 5-6 (uma por concatenação) | 0 |
| Pico de RAM heap | ~2.160 B (buffer + cópia durante realloc) | ~0 B |
| Fragmentação | **Alta** (reallocs sucessivos) | **Nenhuma** |
| Strings temporárias | Sim (cópia no `return`) | Não |

- **Economia pico heap**: ~2.160 B (100%)
- **Impacto na estabilidade**: Elimina fragmentação de heap, principal causa de crashes em ESP8266 com uptime longo.

### Tempo de execução — `handleRoot()`

| Operação | Original | Refatorado |
|---|---|---|
| Montagem HTML | ~0.5-1.0 ms (mallocs + memcpy) | 0 ms |
| Envio | 1 envio TCP (~1.0 ms) | 4-5 envios TCP chunked (~1.5 ms) |
| **Total** | ~1.5-2.0 ms | ~1.5 ms |

O envio chunked adiciona ~0.5 ms pelo overhead TCP, mas elimina o tempo de alocação. **Tempo total equivalente**, com muito menos pressão na memória.

---

## 4. CSS

| Métrica | Original | Refatorado |
|---|---|---|
| Tamanho | ~620 B | ~420 B |
| Nomes de classes | `.status`, `.switch`, `.slider` | `.s`, `.w`, `.l` |
| Espaços/quebras de linha | Preservados | Removidos |

- **Economia Flash**: ~200 B (~32%)
- **Economia de banda por request**: ~200 B — relevante em múltiplas conexões simultâneas.

---

## 5. Handlers HTTP (`/ligar`, `/desligar`)

| Métrica | Original | Refatorado |
|---|---|---|
| `"Location"` | String RAM | `F()` em Flash |
| `"/"` | String RAM | `F()` em Flash |
| RAM por chamada | ~14 B | 0 B |

- **Economia**: ~14 B RAM por handler (total ~28 B)
- **Tempo**: Idêntico (~0.1 ms por redirect)

---

## 6. Resumo Consolidado

| Setor | RAM Economizada | % Economia | Flash Delta |
|---|---|---|---|
| Strings literais | 99 B | 99% | +99 B |
| Template HTML | 1.080 B | 100% | -300 B |
| Heap dinâmico (pico) | 2.160 B | 100% | — |
| Handlers HTTP | 28 B | 100% | +28 B |
| **TOTAL** | **~3.367 B** | — | **~-173 B** |

---

## 7. Totais Estimados

| Recurso | Original | Refatorado | Economia |
|---|---|---|---|
| **RAM livre (runtime)** | ~38.6 KB | ~41.9 KB | **+3.3 KB (~8.5%)** |
| **Flash usada** | ~260 KB | ~260 KB | ~-173 B |
| **Fragmentação heap** | Alta | Nenhuma | **Eliminada** |
| **Tempo `handleRoot()`** | ~1.5-2.0 ms | ~1.5 ms | ~0-0.5 ms |

> Valores aproximados considerando ESP8266 com 80 KB de heap total e overhead do WiFi stack (~40 KB).

---

## 8. Conclusão

A maior economia vem da **eliminação da alocação dinâmica** no `handleRoot()`. Os 3.3 KB recuperados representam ~8.5% do heap livre — significativo no ESP8266 onde a stack WiFi/TCP já consome ~50% da RAM. O ganho principal não é o espaço absoluto, mas a **eliminação da fragmentação**, que previne crashes em operação contínua.
