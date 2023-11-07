import MapIcon from "@mui/icons-material/Map";
import { ListItemButton, ListItemIcon, ListItemText } from "@mui/material";
import List from "@mui/material/List";
import ListSubheader from "@mui/material/ListSubheader";
import * as React from "react";
import { Link } from "react-router-dom";
import { WallpaperSettings } from "../global";
import { Brightness } from "./Brightness";
import { MapUpdateInterval } from "./MapUpdateInterval";
import { Scale } from "./Scale";
import { ScaleType } from "./ScaleType";

type Props = {
  settings: WallpaperSettings;
};

export const Settings: React.FC<Props> = ({ settings }) => (
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
