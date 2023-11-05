import "./App.css";
import { useBridge } from "./useBridge";
import { Button, CircularProgress, Grid } from "@mui/material";
import { Settings } from "./Settings";
import { Header } from "./Header";
import styled from "@emotion/styled";
import CloudUploadIcon from "@mui/icons-material/CloudUpload";

const VisuallyHiddenInput = styled("input")({
  clip: "rect(0 0 0 0)",
  clipPath: "inset(50%)",
  height: 1,
  overflow: "hidden",
  position: "absolute",
  bottom: 0,
  left: 0,
  whiteSpace: "nowrap",
  width: 1,
});

export function App() {
  const { settings } = useBridge();
  const isLoading = !settings;

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

            <Button
              component="label"
              variant="contained"
              startIcon={<CloudUploadIcon />}
            >
              Upload file
              <VisuallyHiddenInput
                type="file"
                onChange={(e) => console.log("on change", e)}
              />
            </Button>
          </Grid>
        )}
      </Grid>
    </Grid>
  );
}
