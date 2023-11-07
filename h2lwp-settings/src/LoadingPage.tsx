import { CircularProgress, Grid } from "@mui/material";
import { Header } from "./Header";

export const LoadingPage = () => (
  <>
    <Header title="Loading..." />

    <Grid
      container
      alignContent="center"
      justifyContent="center"
      flex="1"
      minHeight={400}
      direction="column"
    >
      <CircularProgress />
    </Grid>
  </>
);
