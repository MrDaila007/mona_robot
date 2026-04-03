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
Цель: Математическая проверка фильтра сглаживания.
Так как математика скрыта внутри приватных методов SafetyNode,
тестировать её снаружи через ROS-топики долго.
Идеально — вынести математику фильтра в отдельный класс EmaFilter
(или заголовочный файл), который не зависит от ROS 2,
и написать для него чистые C++ Unit-тесты:
- Подали резкий скачок 1.0.
- Проверили, что выход стал 0.05 (согласно alpha_ = 0.05).
*/
