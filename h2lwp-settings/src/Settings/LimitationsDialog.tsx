import Button from "@mui/material/Button";
import Dialog from "@mui/material/Dialog";
import DialogActions from "@mui/material/DialogActions";
import DialogContent from "@mui/material/DialogContent";
import DialogContentText from "@mui/material/DialogContentText";
import DialogTitle from "@mui/material/DialogTitle";
import * as React from "react";

type Props = {
  open: boolean;
  onClose: () => void;
  onConfirm: () => void;
};

export const LimitationsDialog: React.FC<Props> = ({
  open,
  onClose,
  onConfirm,
}) => (
  <Dialog
    open={open}
    onClose={onClose}
    aria-labelledby="alert-dialog-title"
    aria-describedby="alert-dialog-description"
  >
    <DialogTitle id="alert-dialog-title">Please note</DialogTitle>
    <DialogContent>
      <DialogContentText id="alert-dialog-description">
        Some phones don't allow to set live wallpapers on the lock screen or
        don't allow to set live wallpapers at all!
      </DialogContentText>
    </DialogContent>
    <DialogActions>
      <Button onClick={onConfirm} autoFocus>
        OK
      </Button>
    </DialogActions>
  </Dialog>
);
