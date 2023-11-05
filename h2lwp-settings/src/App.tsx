import "./App.css";
import { useBridge } from "./useBridge";
import {
  AppBar,
  Button,
  CircularProgress,
  Grid,
  Toolbar,
  Typography,
} from "@mui/material";
import { Settings } from "./Settings";

export function App() {
  const { state } = useBridge();

  const isLoading = Object.keys(state).length === 0;

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
