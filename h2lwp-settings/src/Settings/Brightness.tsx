import { Grid, ListItem, Slider, Typography } from "@mui/material";
import { useState } from "react";

type Props = {
  value: number;
};

export const Brightness: React.FC<Props> = ({ value: initialValue }) => {
  const [value, setValue] = useState(initialValue);

  return (
    <ListItem>
      <Grid direction="column" width="100%">
        <Typography gutterBottom>Brightness</Typography>

        <Slider
          aria-label="Brightness"
          value={value}
          onChange={(_, value) => setValue(Number(value))}
        />
      </Grid>
    </ListItem>
  );
};
