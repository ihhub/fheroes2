import React from "react";
import ReactDOM from "react-dom/client";
import "./index.css";
import App from "./App";
import reportWebVitals from "./reportWebVitals";

function startApp(callback: () => void) {
  if (window.Bridge.initialized) {
    callback();
  } else {
    window.Bridge.afterInitialize = callback;
  }
}

window.Bridge.init();

startApp(() => {
  // @ts-ignore
  window.Android = window.Bridge.interfaces.Android;

  const root = ReactDOM.createRoot(
    document.getElementById("root") as HTMLElement
  );
  root.render(
    <React.StrictMode>
      <App />
    </React.StrictMode>
  );
});

// If you want to start measuring performance in your app, pass a function
// to log results (for example: reportWebVitals(console.log))
// or send to an analytics endpoint. Learn more: https://bit.ly/CRA-vitals
reportWebVitals();
