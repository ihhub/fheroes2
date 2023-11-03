import React, { Suspense } from "react";
import logo from "./logo.svg";
import "./App.css";
import { suspend } from "suspend-react";

const Child = () => {
  const version = "v0";
  const id = "1000";

  const data = suspend(async () => {
    // const res = await fetch(
    //   `https://hacker-news.firebaseio.com/${version}/item/${id}.json`
    // );

    const res = new Promise(resolve => {
      setTimeout(() => resolve({
        title: 'loaded data',
        by: 'by smthing'
      }), 6000);
    });

    return res;
  }, [id, version]);

  return (
    <div>
    {/* @ts-ignore */}
      {data.title} by {data.by}
    </div>
  );
};

function App() {
  return (
    <div className="App">
      <header className="App-header">
        <img src={logo} className="App-logo" alt="logo" />
        <p>
          Edit <code>src/App.tsx</code> and save to reload.
        </p>
        <a
          className="App-link"
          href="https://reactjs.org"
          target="_blank"
          rel="noopener noreferrer"
        >
          Learn React
        </a>

        <Suspense fallback={<div>Loading...</div>}>
          <Child />
        </Suspense>
      </header>
    </div>
  );
}

export default App;
