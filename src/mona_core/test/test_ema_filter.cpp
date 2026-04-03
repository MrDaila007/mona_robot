// Copyright 2026 vladubase
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


/*
Цель: Проверить маршрутизацию и таймауты (те самые 5 секунд блокировки Nav2).
Как тестируется:
- Создаете тестовую ноду (Fixture).
- Публикуете сообщение в /cmd_teleop.
- Сразу публикуете сообщение в /cmd_nav.

Подписываетесь на /cmd_vel и проверяете через GTest (EXPECT_EQ), что на моторы ушла команда от телеопа, а навигация была проигнорирована.
*/
