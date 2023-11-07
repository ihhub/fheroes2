import ArrowBackIcon from "@mui/icons-material/ArrowBack";
import { AppBar, Grid, IconButton, Toolbar, Typography } from "@mui/material";
import { ReactNode } from "react";
import { useNavigate } from "react-router-dom";

type Props = {
  title: ReactNode;
  backButton?: boolean;
};

export const Header: React.FC<Props> = ({ title, backButton }) => {
  const navigate = useNavigate();

  return (
    <Grid>
      <AppBar>
        <Toolbar>
          {backButton && (
            <IconButton
              size="large"
              edge="start"
              color="inherit"
              aria-label="menu"
              sx={{ mr: 2 }}
              onTouchEnd={() => navigate("..")}
            >
              <ArrowBackIcon />
            </IconButton>
          )}
          <Typography variant="h6" component="div">
            {title}
          </Typography>
        </Toolbar>
      </AppBar>

      <Toolbar />
    </Grid>
  );
};
