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

tlib.main = () => {
    function loop() {
        try {
            tlib.import.os_main(0, 0)
            let timeout = Number(tlib.next_sleep) / 1000
            window.setTimeout(loop, timeout)
        } catch(error) {
            // Exit called
            if(!tlib.exit) {
                console.log("FAIL: ", error);
                alert(error)
            }
        }
    }

    fetch('index.wasm')
        .then(re => re.arrayBuffer())
        .then(data => WebAssembly.instantiate(data, { env: tlib.export }))
        .then(ret => {
            tlib.memory = ret.instance.exports.memory;
            tlib.import = ret.instance.exports;
            loop()
        });
}

tlib.export.wasm_exit = () => {
    tlib.exit = true
}

tlib.export.wasm_fail = (data, len) => {
    // data is a pointer in wasm memory
    var bytes = new Uint8Array(tlib.memory.buffer, data, len)

    // Convert utf8 bytes array to a js string
    var string = new TextDecoder('utf8').decode(bytes)

    // Show alert dialog
    alert(string);
    console.log(string);
}

// Write utf8 text to the console
tlib.export.wasm_write = (fd, data, len) => {
    // data is a pointer in wasm memory
    var bytes = new Uint8Array(tlib.memory.buffer, data, len)

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
