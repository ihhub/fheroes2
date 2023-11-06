import { Button, CircularProgress, Grid } from "@mui/material";
import { useEffect } from "react";
import "./App.css";
import { FileList } from "./FileList";
import { Header } from "./Header";
import { Settings } from "./Settings";
import { useBridge } from "./useBridge";

export function App() {
  const { settings, ready } = useBridge();
  const isLoading = !settings || !ready;

  useEffect(() => {
    console.log({
      isLoading,
      ready,
      settings,
      a: window.Android,
    });
  }, [isLoading, ready, settings]);

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

        {!isLoading && <Settings settings={settings} />}

        {!isLoading && (
          <Grid
            container
            alignContent="center"
            justifyContent="flex-start"
            flex="1"
            flexShrink={1}
            padding={2}
            gap={2}
            direction="column"
          >
            <Button
              variant="contained"
              onClick={() => window.Android?.setWallpaper()}
            >
              Set wallpaper
            </Button>

            <FileList />
          </Grid>
        )}
      </Grid>
    </Grid>
  );
}
