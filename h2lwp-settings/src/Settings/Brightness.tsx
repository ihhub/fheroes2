import { Box, Grid, ListItem, Slider, Typography } from "@mui/material";
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

        <Box marginX={2}>
          <Slider
            aria-label="Brightness"
            value={value}
            min={0}
            max={1}
            step={0.01}
            valueLabelDisplay="auto"
            valueLabelFormat={(v) => Math.round(Number(v) * 100) + "%"}
            onChange={(_, value) => setValue(Number(value))}
            sx={{
              p: 0,
            }}
          />
        </Box>
      </Grid>
    </ListItem>
  );
};
