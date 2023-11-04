import * as React from "react";
import List from "@mui/material/List";
import ListSubheader from "@mui/material/ListSubheader";
import { Scale } from "./Settings/Scale";
import { ScaleType } from "./Settings/ScaleType";
import { Brightness } from "./Settings/Brightness";
import { UseScroll } from "./Settings/UseScroll";
import { MapUpdateInterval } from "./Settings/MapUpdateInterval";

export const Settings: React.FC<{ state: Record<string, string> }> = ({
  state,
}) => {
  // Scale
  // ScaleType
  // Map update interval
  // Brightness

  return (
    <List subheader={<ListSubheader>Settings</ListSubheader>}>
      <ScaleType value={state.scaleType} />

      <Scale value={Number(state.scale)} />

      <MapUpdateInterval value={Number(state.mapUpdateInterval)} />

      <Brightness value={Number(state.brightness)} />

      <UseScroll value={Boolean(state.useMapScroll)} />
    </List>
  );
};
