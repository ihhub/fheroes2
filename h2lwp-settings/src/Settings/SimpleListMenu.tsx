import * as React from "react";
import List from "@mui/material/List";
import ListItemText from "@mui/material/ListItemText";
import MenuItem from "@mui/material/MenuItem";
import Menu from "@mui/material/Menu";
import { ListItemButton } from "@mui/material";

type Item<T> = {
  value: T;
  title: string;
};

type Props<T> = {
  label: string;
  items: Item<T>[];
  value: T;
  onChange: (value: T) => void;
};

export const SimpleListMenu = <T extends number | string>({
  label,
  items,
  onChange,
  value,
}: Props<T>) => {
  const [anchorEl, setAnchorEl] = React.useState<null | HTMLElement>(null);
  const open = Boolean(anchorEl);

  return (
    <div>
      <List disablePadding component="nav" sx={{ bgcolor: "background.paper" }}>
        <ListItemButton
          aria-haspopup="listbox"
          aria-expanded={open ? "true" : undefined}
          onClick={(event) => setAnchorEl(event.currentTarget)}
        >
          <ListItemText
            primary={label}
            secondary={
              items.find((item) => item.value === value)?.title ?? "\u00a0"
            }
          />
        </ListItemButton>
      </List>
      <Menu
        anchorEl={anchorEl}
        open={open}
        MenuListProps={{ role: "listbox" }}
        onClose={() => setAnchorEl(null)}
      >
        {items.map((item) => (
          <MenuItem
            key={item.value}
            selected={item.value === value}
            onClick={() => {
              onChange(item.value);
              setAnchorEl(null);
            }}
            sx={{ minWidth: 200 }}
          >
            {item.title}
          </MenuItem>
        ))}
      </Menu>
    </div>
  );
};
