function startApp(f) {
    if (Bridge.initialized) {
        f()
    } else {
        Bridge.afterInitialize = f
    }
}

Bridge.init()

startApp(() => {
    console.log("app started")
    console.log(Bridge.interfaces.Android.helloFullSync("helloFullSync"))
    console.log(Bridge.interfaces.Android.helloFullPromise("helloFullPromise").then(a => console.log("response:",a)))
});
