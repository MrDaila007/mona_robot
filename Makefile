# ==============================================================================
# Copyright 2026 vladubase
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

COMPOSE ?= docker compose

.PHONY: build rebuild up up-gpu down logs ps shell format ci help \
	b r u ug d l p s f c

help: ## Show available Makefile targets
	@echo "Available commands:"
	@grep -E '^[a-zA-Z0-9_-]+:.*##' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS=":.*## "}; {printf "  \033[1;32m%-15s\033[0m %s\n", $$1, $$2}'

build: ## Build docker images (uses cache, fast)
	$(COMPOSE) build

rebuild: ## Full clean build of docker images (no-cache) and restart
	$(COMPOSE) build --no-cache

up: ## Start dev container (CPU-only, builds if missing)
	$(COMPOSE) up -d --build

up-gpu: ## Start dev container with NVIDIA GPU
	$(COMPOSE) -f docker-compose.yml -f docker-compose.gpu.yml up -d --build

down: ## Stop containers
	$(COMPOSE) down

logs: ## Tail docker compose logs
	$(COMPOSE) logs -f

ps: ## List docker compose services
	$(COMPOSE) ps

shell: ## Attach to the dev container's bash shell
	$(COMPOSE) up -d dev
	$(COMPOSE) exec -it dev bash

format: ## Run auto-formatting inside the dev container
	$(COMPOSE) up -d dev
	$(COMPOSE) exec dev bash -c "./scripts/format_code.bash"

ci: ## Run CI checks inside the dev container
	$(COMPOSE) up -d dev
	$(COMPOSE) exec dev bash -c "./scripts/run_ci_checks.bash"

world: ## Start Gazebo world (run on host)
	./scripts/start_world.bash

robot: ## Start robot agent (run on host)
	./scripts/start_1_robot.bash

# Short aliases for fast TAB completion
b: ## alias: build
	$(MAKE) build

r: ## alias: rebuild
	$(MAKE) rebuild

u: ## alias: up
	$(MAKE) up

ug: ## alias: up-gpu
	$(MAKE) up-gpu

d: ## alias: down
	$(MAKE) down

l: ## alias: logs
	$(MAKE) logs

p: ## alias: ps
	$(MAKE) ps

s: ## alias: shell
	$(MAKE) shell

f: ## alias: format
	$(MAKE) format

c: ## alias: ci
	$(MAKE) ci

w: ## alias: world
	$(MAKE) world

ro: ## alias: robot
	$(MAKE) robot