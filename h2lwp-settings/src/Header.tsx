import { AppBar, Toolbar, Typography } from "@mui/material";
import { ReactNode } from "react";

type Props = {
  title: ReactNode;
};

export const Header: React.FC<Props> = ({ title }) => (
  <>
    <AppBar>
      <Toolbar>
        <Typography variant="h6" component="div">
          {title}
        </Typography>
      </Toolbar>
    </AppBar>

    <Toolbar />
  </>
);
