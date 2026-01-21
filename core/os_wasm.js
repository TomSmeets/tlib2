// Copyright (c) 2025 - Tom Smeets <tom@tsmeets.nl>
// os_base_wasm.js - Base OS implementation for WASM
var tlib = {
    // Wasm memory
    memory: null,

    // Symbols defined in c exported to js
    imports: {},

    // Symbols defined in js exported to c
    exports: {},
};

tlib.imports.os_main();

tlib.export.wasm_exit = () => {
    tlib.exit = true
}

// Write utf8 text to the console
tlib.export.wasm_write = (fd, data, len) => {
    // data is a pointer in wasm memory
    var bytes = new Uint8Array(ctx.memory.buffer, data, len)

    // Convert utf8 bytes array to a js string
    var string = new TextDecoder('utf8').decode(bytes)

    // Log to the console.
    console.log(string)

    return true
}

// Time in micro seconds
tlib.export.wasm_time = () => {
    return BigInt(new Date().getTime()*1000)
}

// Time in micro seconds
tlib.export.wasm_sleep = (duration) => {
    tlib.next_sleep = duration
}

tlib.export.wasm_system = (command) => {
    try {
        eval(command) 
    } catch (e) {
        return 1
    }
    return 0
}
