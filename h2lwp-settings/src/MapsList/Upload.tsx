import { Button, styled } from "@mui/material";

const VisuallyHiddenInput = styled("input")({
  clip: "rect(0 0 0 0)",
  clipPath: "inset(50%)",
  height: 1,
  overflow: "hidden",
  position: "absolute",
  bottom: 0,
  left: 0,
  whiteSpace: "nowrap",
  width: 1,
});

type Props = {
  onFileSelect?: (isSupported: boolean) => void;
};

export const Upload: React.FC<Props> = () => (
  <Button variant="contained" component="label">
    Add map
    <VisuallyHiddenInput type="file" />
  </Button>
);
