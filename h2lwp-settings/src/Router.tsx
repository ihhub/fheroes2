import { ReactNode } from "react";
import { MemoryRouter } from "react-router-dom";

type Props = {
  children?: ReactNode;
};

export const Router: React.FC<Props> = ({ children }) => (
  <MemoryRouter initialEntries={["/"]} initialIndex={0}>
    {children}
  </MemoryRouter>
);
