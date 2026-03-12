tlib.export.pix_wasm_start_audio = (buffer_data_ptr, buffer_size, cursor_ptr) => {
    if(!tlib.audio_init) {
        const buffer_data = new Float32Array(tlib.memory.buffer, buffer_data_ptr, buffer_size*2);
        const cursor      = new Uint32Array(tlib.memory.buffer, cursor_ptr, 1);
        const sample_count = 1024;
        tlib.audio = new AudioContext();
        tlib.audio_processor = tlib.audio.createScriptProcessor(sample_count, 2, 2);
        tlib.audio_processor.onaudioprocess = function (ev) {
            const chan_0 = ev.outputBuffer.getChannelData(0);
            const chan_1 = ev.outputBuffer.getChannelData(1);
            for(let i = 0; i < sample_count; ++i) {
                const c0 = cursor[0]*2 + 0;
                const c1 = cursor[0]*2 + 1;

                // Read sample
                chan_0[i] = buffer_data[c0];
                chan_1[i] = buffer_data[c1];

                // Clear sample
                buffer_data[c0] = 0;
                buffer_data[c1] = 0;

                // Advance cursor
                cursor[0] += 1;
                if (cursor[0] >= buffer_size) cursor[0] = 0;
            }
        }
        tlib.audio_processor.connect(tlib.audio.destination);
        tlib.audio_init = true;
    }
    tlib.audio.resume();
}

tlib.export.pix_wasm_draw = (size_x, size_y, data_ptr) => {
    var data = new Uint8ClampedArray(tlib.memory.buffer, data_ptr, size_x*size_y*4);
    let image_data = new ImageData(data, size_x, size_y);

    if(!tlib.canvas) {
        tlib.canvas = document.getElementById("canvas");
        tlib.canvas.width = size_x
        tlib.canvas.height = size_y
        tlib.ctx = tlib.canvas.getContext("2d");
    }

    if(tlib.canvas.width != size_x || tlib.canvas.height != size_y) {
        tlib.canvas.width = size_x
        tlib.canvas.height = size_y
    }
    tlib.ctx.putImageData(image_data, 0, 0);
}

tlib.export.pix_wasm_start_input = (pix) => {
    document.addEventListener("keydown",   (ev) => { if (!ev.repeat) tlib.import.pix_wasm_key_down(pix, ev.keyCode, true) })
    document.addEventListener("keyup",     (ev) => { if (!ev.repeat) tlib.import.pix_wasm_key_down(pix, ev.keyCode, false) })
    document.addEventListener('contextmenu', (ev) => { ev.preventDefault() });
}
