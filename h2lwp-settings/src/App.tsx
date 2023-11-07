import { Grid } from "@mui/material";
import { HashRouter, Route, Routes } from "react-router-dom";
import "./App.css";
import { LoadingPage } from "./LoadingPage";
import { MapsListPage } from "./MapsListPage";
import { SettingsPage } from "./SettingsPage";
import { useBridge } from "./useBridge";

export function App() {
  const { settings, ready } = useBridge();
  const isLoading = !settings || !ready;

  return (
    <HashRouter>
      <Grid container direction="column">
        <Routes>
          {isLoading && <Route path="*" element={<LoadingPage />} />}

          {!isLoading && (
            <>
              <Route path="/maps-list" element={<MapsListPage />} />
              <Route path="*" element={<SettingsPage settings={settings} />} />
            </>
          )}
        </Routes>
      </Grid>
    </HashRouter>
  );
}
