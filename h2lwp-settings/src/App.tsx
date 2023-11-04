import { Suspense, useEffect } from "react";
import "./App.css";
import { useBridge } from "./useBridge";
import {
  AppBar,
  Box,
  Button,
  Container,
  Toolbar,
  Typography,
} from "@mui/material";
import { Settings } from "./Settings";
// import { suspend } from "suspend-react";

const Child: React.FC<{ state: unknown }> = ({ state }) => {
  // const data = suspend(
  //   async () => window.Android.helloFullPromise("helloFullPromise"),
  //   []
  // );

  useEffect(() => {
    setTimeout(() => {
      console.log("setBrightness");
      window.Android?.setBrightness(80);
    }, 3000);
  }, []);

  return <pre>{JSON.stringify(state, null, 2)}</pre>;
};

export function App() {
  const { state } = useBridge();

  return (
    <>
      <AppBar>
        <Toolbar>
          <Typography variant="h6" component="div">
            Wallpaper settings
          </Typography>
        </Toolbar>
      </AppBar>

      <Toolbar />

        <Settings state={state} />

        <Box>
          <Button
            variant="contained"
            onClick={() => window.Android?.setWallpaper()}
          >
            Set wallpaper
          </Button>

          <Suspense fallback={<div>Loading...</div>}>
            <Child state={state} />
          </Suspense>
        </Box>
    </>
  );
}
