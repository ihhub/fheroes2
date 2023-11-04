import "./App.css";
import { useBridge } from "./useBridge";
import { AppBar, Button, Grid, Toolbar, Typography } from "@mui/material";
import { Settings } from "./Settings";

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

      <Grid
        container
        alignContent="center"
        justifyContent="center"
        padding={2}
        width="100%"
      >
        <Button
          variant="contained"
          onClick={() => window.Android?.setWallpaper()}
        >
          Set wallpaper
        </Button>
      </Grid>
    </>
  );
}
