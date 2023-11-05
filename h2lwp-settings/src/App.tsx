import "./App.css";
import { useBridge } from "./useBridge";
import { Button, CircularProgress, Grid } from "@mui/material";
import { Settings } from "./Settings";
import { Header } from "./Header";

export function App() {
  const { state } = useBridge();
  const isLoading = Object.keys(state).length === 0;

  return (
    <Grid container height="100%" width="100%" direction="column">
      <Header title="Wallpaper settings" />

      <Grid
        container
        alignContent="center"
        justifyContent="center"
        flex="1"
        direction="column"
      >
        {isLoading && <CircularProgress />}

        {!isLoading && <Settings state={state} />}

        {!isLoading && (
          <Grid
            container
            alignContent="flex-start"
            justifyContent="center"
            flex="1"
            flexShrink={1}
            padding={2}
          >
            <Button
              variant="contained"
              onClick={() => window.Android?.setWallpaper()}
            >
              Set wallpaper
            </Button>
          </Grid>
        )}
      </Grid>
    </Grid>
  );
}
