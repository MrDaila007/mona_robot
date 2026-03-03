COMPOSE ?= docker compose

.PHONY: up up-gpu down logs ps rebuild help \
	u ug d l p r

help: ## Show available Makefile targets
	@echo "Available commands:"
	@grep -E '^[a-zA-Z0-9_-]+:.*##' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS=":.*## "}; {printf "  \033[1;32m%-12s\033[0m %s\n", $$1, $$2}'

up: ## Start dev container (CPU-only)
	$(COMPOSE) up -d --build

up-gpu: ## Start dev container with NVIDIA GPU
	$(COMPOSE) -f docker-compose.yml -f docker-compose.gpu.yml up -d --build

down: ## Stop containers
	$(COMPOSE) down

logs: ## Tail docker compose logs
	$(COMPOSE) logs -f

ps: ## List docker compose services
	$(COMPOSE) ps

rebuild: ## Full rebuild and start
	$(COMPOSE) down
	$(COMPOSE) up -d --build

# Short aliases for fast TAB completion
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

r: ## alias: rebuild
	$(MAKE) rebuild

