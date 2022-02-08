import { PrimitiveAtom, atom } from "jotai";

import {
  MeasureWrapper,
  PaxForecastApplyMeasuresResponse,
} from "../api/protocol/motis/paxforecast";

export const universeAtom = atom(0);
export const scheduleAtom = atom(0);

export interface SimulationResult {
  universe: number;
  startedAt: Date;
  finishedAt: Date;
  measures: MeasureWrapper[];
  response: PaxForecastApplyMeasuresResponse;
}

export const simResultsAtom = atom<PrimitiveAtom<SimulationResult>[]>([]);
