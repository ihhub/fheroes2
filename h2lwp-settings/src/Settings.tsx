import * as React from "react";
import List from "@mui/material/List";
import ListSubheader from "@mui/material/ListSubheader";
import { Scale } from "./Settings/Scale";
import { ScaleType } from "./Settings/ScaleType";
import { Brightness } from "./Settings/Brightness";
import { MapUpdateInterval } from "./Settings/MapUpdateInterval";
import { WallpaperSettings } from "./global";

type Props = {
  settings: WallpaperSettings;
};

export const Settings: React.FC<Props> = ({ settings }) => (
  <List subheader={<ListSubheader>Settings</ListSubheader>} disablePadding>
    <ScaleType value={settings.scaleType} />

    <Scale value={settings.scale} />

    <MapUpdateInterval value={settings.mapUpdateInterval} />

    <Brightness value={settings.brightness} />
  </List>
);
