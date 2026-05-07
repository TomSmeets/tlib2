
cache = {}
function eval_cached(code) {
    if(!(code in cache)) cache[code] = eval(code)
    return cache[code]
}

function time_s() {
    return new Date().getTime()
}

function time_d() {
    return eval("new Date().getTime()")
}

function time_d2() {
    return eval_cached("() => new Date().getTime()")()
}


// js function -> single 'int' descriptor
// js_define(code) => i
// js_call(code)      =>
cache2 = {}
function define(code) {
    // C
    if (!(code in cache)) {
    }

    return code
}

function measure(f) {
  let t_start = f();
  let t_now   = t_start;
  let i = 0;
  while(t_now < t_start + 1 * 1000) {
      t_now = f();
      i += 1
  }
  return i
}

function test() {
  // let base = measure(time_s)
  // console.log("Static:   " + (measure(time_s)  / base).toFixed(2))
  // console.log("Dynamic1: " + (measure(time_d)  / base).toFixed(2))
  // console.log("Dynamic2: " + (measure(time_d2) / base).toFixed(2))

  let i = 0;
  let t0 = time_s()

  let j = 0
  while(j < 1000*1000) {
      i = eval_cached("(x) => x*x*x + 1")(i);
      i = eval_cached("(x) => x*2 + 1")(i);
      i = eval_cached("(x) => x*5 + 1")(i);
      i = eval_cached("(x) => x*3 + 1")(i);
      j = j + 1;
  }
  let t1 = time_s()
  console.log(t1 - t0)
}

test()
