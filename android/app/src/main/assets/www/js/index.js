function showToast(){
    var message = document.getElementById("message").value;
    var lengthLong = document.getElementById("length").checked;

    console.log(Android.makeToast(message, lengthLong));
    return false;
}
//
//async function callAndroidAsync(javaFuncName, params) {
//	const rand = 'asyncJava_' + Math.floor(Math.random() * 1000000)
//	window[rand] = {}
//
//	// func called from android
//	window[rand].callback = (isSuccess) => {
//        const dataOrErr = Android.runAsyncResult(rand)
//		if (isSuccess) window[rand].resolve(dataOrErr)
//		else window[rand].reject(dataOrErr)
//		delete window[rand] // clean up
//	}
//
//	// call some android function that returns immediately - should run in a new thread
//	// setTimeout(() => window[rand].callback(false, params.val * 2), 4000) // see testCallJavaAsync
//	Android.runAsync(rand, javaFuncName, JSON.stringify(params))
//
//	return new Promise((resolve, reject) => {
//		window[rand].resolve = (data) => resolve(data)
//		window[rand].reject = (err) => reject(err)
//	})
//}
//
//function testAsync() {
//     async function testCallJavaAsync() {
//        const res = await callAndroidAsync('testFunc', { val: 100 })
//        console.log(`received res = ${res}`)
//     }
//
//     testCallJavaAsync()
//}
//
//window.testAsync = testAsync;

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
    console.log(Bridge.interfaces.Android.helloFullSync("Web"))

    var form = document.getElementById("form");
    form.onsubmit = showToast;
});


