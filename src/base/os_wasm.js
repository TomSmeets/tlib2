// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_base_wasm.js - Base OS implementation for WASM
var tlib = {
    // Wasm memory
    memory: null,

    // Symbols defined in c exported to js
    import: {},

    // Symbols defined in js exported to c
    export: {},
};

tlib.main = (data) => {
    function loop() {
        try {
            tlib.import.os_main_wrapper(0, 0)
            let timeout = Number(tlib.next_sleep) / 1000
            window.setTimeout(loop, timeout)
        } catch(error) {
            if(tlib.exit) {
                // Exit was called
            } else {
                // Actual exception
                console.log("FAIL: ", error);
                alert(error)
            }
        }
    }

    WebAssembly.instantiate(data, { env: tlib.export }).then(ret => {
        tlib.memory = ret.instance.exports.memory;
        tlib.import = ret.instance.exports;
        loop()
    });
}

// Decode a C String
const text_decoder = new TextDecoder("utf-8");

// C-string to js string
function str_c(ptr) {
    const memory = new Uint8Array(tlib.memory.buffer);
    let end = ptr;
    while (memory[end] !== 0) end++;
    return text_decoder.decode(memory.subarray(ptr, end));
}

// C-buffer to js string
function str_buf(ptr, len) {
    const memory = new Uint8Array(tlib.memory.buffer, ptr, len);
    return text_decoder.decode(memory);
}

let code_cache = {}
function wasm_eval(code, args) {
    // Check if this function was already compiled
    if (!(code in code_cache)) {
        let str = str_c(code)
        console.log("Defining: ", str)
        // Compile "(x,y,z) => ..." to a js function and store it
        code_cache[code] = eval(str)
    }

    return code_cache[code](...args)
}

let args = []
tlib.export.js_push     = x => args.push(x);
tlib.export.js_push_str = x => args.push(str_c(x));
tlib.export.js_call     = c => {
    ret = wasm_eval(c, args);
    args = [];
    return ret;
}
