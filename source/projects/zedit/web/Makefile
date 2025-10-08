
BUILD_DIR=public
JS=$(BUILD_DIR)/js
CFLAGS=-std=c99 -Wall

all: $(JS)/editor.min.js

node_modules:
	@npm i rollup @rollup/plugin-node-resolve
	@npm i rollup-plugin-import-css
	@npm i @rollup/plugin-commonjs
	@npm i terser


editor.mjs: node_modules
	@echo "OK"

$(JS)/editor.min.js: $(JS)/editor.bundle.js
	@node_modules/terser/bin/terser $(JS)/editor.bundle.js -o $(JS)/editor.min.js -c

$(JS)/editor.bundle.js: editor.mjs $(BUILD_DIR)/index.html
	@echo "Bundling js"
	@node_modules/.bin/rollup -c
	@npx prettier --tab-width 4 --write editor.mjs

$(BUILD_DIR)/index.html: index.html
	@echo "copy index.html -> ./public"
	@cp index.html $(BUILD_DIR)/index.html

index.html:
	@echo "index.html changes"


PHONY: clean reset

clean:
	@rm -rf build

reset: clean
	@rm -rf node_modules package.json package-lock.json
