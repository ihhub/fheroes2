import MapIcon from "@mui/icons-material/Map";
import { ListItemButton, ListItemIcon, ListItemText } from "@mui/material";
import List from "@mui/material/List";
import ListSubheader from "@mui/material/ListSubheader";
import { Link } from "react-router-dom";
import { useBridgeContext } from "../BridgeContext";
import { Brightness } from "./Brightness";
import { MapUpdateInterval } from "./MapUpdateInterval";
import { Scale } from "./Scale";
import { ScaleType } from "./ScaleType";

export const Settings = () => {
  const { settings } = useBridgeContext();

  if (!settings) {
    return null;
  }

  return (
    <List subheader={<ListSubheader>Settings</ListSubheader>} disablePadding>
      <ListItemButton component={Link} to="/maps-list">
        <ListItemIcon>
          <MapIcon />
        </ListItemIcon>

        <ListItemText primary="Maps list" />
      </ListItemButton>

      <ScaleType value={settings.scaleType} />

      <Scale value={settings.scale} />

      <MapUpdateInterval value={settings.mapUpdateInterval} />

      <Brightness value={settings.brightness} />
    </List>
  );
};
