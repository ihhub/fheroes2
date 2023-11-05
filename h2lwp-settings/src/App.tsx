import "./App.css";
import { useBridge } from "./useBridge";
import { Button, CircularProgress, Grid } from "@mui/material";
import { Settings } from "./Settings";
import { Header } from "./Header";

export function App() {
  const { state } = useBridge();
  const isLoading = Object.keys(state).length === 0;

  return (
    <>
      <Header title="Wallpaper settings" />

      {isLoading && (
        <Grid
          height="100%"
          container
          alignContent="center"
          justifyContent="center"
        >
          <CircularProgress />
        </Grid>
      )}

      {!isLoading && (
        <Grid>
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
        </Grid>
      )}
    </>
  );
}
