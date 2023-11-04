import { Suspense, useEffect } from "react";
import "./App.css";
import { useBridge } from "./useBridge";
import { Box, Button, Container, Typography } from "@mui/material";
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
    <Container maxWidth="sm">
      <Box sx={{ my: 4 }}>
        <Typography variant="h4" component="h1" gutterBottom>
          Material UI Create React App example in TypeScript
        </Typography>

        <Button onClick={() => window.Android?.setWallpaper()}>
          Set wallpaper
        </Button>

        <Suspense fallback={<div>Loading...</div>}>
          <Child state={state} />
        </Suspense>
      </Box>
    </Container>
  );
}
