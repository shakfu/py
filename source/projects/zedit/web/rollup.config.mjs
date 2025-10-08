import {nodeResolve} from "@rollup/plugin-node-resolve"
import commonjs from '@rollup/plugin-commonjs';
import css from "rollup-plugin-import-css";

export default {
    input: "./editor.mjs",
    output: {
    file: "./public/js/editor.bundle.js",
    format: "iife"
    },
    plugins: [
        nodeResolve(), 
        css(),
        commonjs(),
    ]
}
