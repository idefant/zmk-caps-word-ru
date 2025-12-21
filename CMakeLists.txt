# SPDX-License-Identifier: MIT

# Если у тебя есть папка include/ (например, include/dt-bindings/...), оставь строку ниже.
# Если include/ нет — можешь либо создать пустую include/, либо удалить эту строку.
# target_include_directories(app PRIVATE include)

# Обычно behaviors выполняются на unibody или на центральной половинке сплита:
if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE src/behaviors/behavior_caps_word_ru.c)
endif()
