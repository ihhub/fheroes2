import * as React from "react";
import List from "@mui/material/List";
import ListItem from "@mui/material/ListItem";
import ListItemIcon from "@mui/material/ListItemIcon";
import ListItemText from "@mui/material/ListItemText";
import ListSubheader from "@mui/material/ListSubheader";
import Switch from "@mui/material/Switch";
import WifiIcon from "@mui/icons-material/Wifi";
import BluetoothIcon from "@mui/icons-material/Bluetooth";
import { Grid, ListItemButton, Slider, Typography } from "@mui/material";
import { Scale } from "./Settings/Scale";

export const Settings: React.FC<{ state: Record<string, string> }> = ({
  state,
}) => {
  const [checked, setChecked] = React.useState(["wifi"]);

  const handleToggle = (value: string) => () => {
    const currentIndex = checked.indexOf(value);
    const newChecked = [...checked];

    if (currentIndex === -1) {
      newChecked.push(value);
    } else {
      newChecked.splice(currentIndex, 1);
    }

    setChecked(newChecked);
  };

  const [value, setValue] = React.useState<number>(
    Number(state.brightness) || 0
  );
  const handleChange = (event: Event, newValue: number | number[]) => {
    setValue(newValue as number);
  };

  // Scale
  // ScaleType
  // Map update interval
  // Brightness

  return (
    <List subheader={<ListSubheader>Settings</ListSubheader>}>
      <Scale value={Number(state.scale)} />

      <ListItem>
        <Grid direction="column" width="100%">
          <Typography gutterBottom>Brightness</Typography>

          <Slider
            aria-label="Brightness"
            value={value}
            onChange={handleChange}
          />
        </Grid>
      </ListItem>

      <ListItemButton onClick={handleToggle("wifi")}>
        <ListItemIcon>
          <WifiIcon />
        </ListItemIcon>
        <ListItemText id="switch-list-label-wifi" primary="Wi-Fi" />
        <Switch
          edge="end"
          onChange={handleToggle("wifi")}
          checked={checked.indexOf("wifi") !== -1}
          inputProps={{
            "aria-labelledby": "switch-list-label-wifi",
          }}
        />
      </ListItemButton>

      <ListItemButton onClick={handleToggle("bluetooth")}>
        <ListItemIcon>
          <BluetoothIcon />
        </ListItemIcon>
        <ListItemText id="switch-list-label-bluetooth" primary="Bluetooth" />
        <Switch
          edge="end"
          onChange={handleToggle("bluetooth")}
          checked={checked.indexOf("bluetooth") !== -1}
          inputProps={{
            "aria-labelledby": "switch-list-label-bluetooth",
          }}
        />
      </ListItemButton>
    </List>
  );
};
