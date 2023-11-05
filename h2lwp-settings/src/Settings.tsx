import * as React from "react";
import List from "@mui/material/List";
import ListSubheader from "@mui/material/ListSubheader";
import { Scale } from "./Settings/Scale";
import { ScaleType } from "./Settings/ScaleType";
import { Brightness } from "./Settings/Brightness";
// import { UseScroll } from "./Settings/UseScroll";
import { MapUpdateInterval } from "./Settings/MapUpdateInterval";

export const Settings: React.FC<{ state: Record<string, unknown> }> = ({
  state,
}) => (
  <List subheader={<ListSubheader>Settings</ListSubheader>} disablePadding>
    <ScaleType value={Number(state.scaleType) || 0} />

    <Scale value={Number(state.scale) || 0} />

    <MapUpdateInterval value={Number(state.mapUpdateInterval) || 0} />

    <Brightness value={Number(state.brightness) || 0} />

    {/*<UseScroll value={Boolean(state.useMapScroll)} />*/}
  </List>
);
