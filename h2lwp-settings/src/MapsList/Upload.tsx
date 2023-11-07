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
  onFileSelect: (isSupported: boolean) => void;
};

export const Upload: React.FC<Props> = ({ onFileSelect }) => (
  <Button variant="contained" component="label">
    Upload
    <VisuallyHiddenInput
      type="file"
      onChange={(e) => {
        // File will be returned from WebView only if it accepted by backend
        const file = e.currentTarget?.files?.[0];

        onFileSelect(Boolean(file));
      }}
    />
  </Button>
);
