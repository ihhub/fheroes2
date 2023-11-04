import { ListItemButton, ListItemText, Switch } from "@mui/material";
import { useState } from "react";

type Props = {
  value: boolean;
};

export const UseScroll: React.FC<Props> = ({ value: initialValue }) => {
  const [value, setValue] = useState(initialValue);

  return (
    <ListItemButton onClick={() => setValue((v) => !v)}>
      <ListItemText primary="Use map scroll" />

      <Switch edge="end" onClick={() => setValue((v) => !v)} checked={value} />
    </ListItemButton>
  );
};
