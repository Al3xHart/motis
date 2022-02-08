import { verifyContentType } from "./protocol/checks";
import {
  PaxForecastApplyMeasuresRequest,
  PaxForecastApplyMeasuresResponse,
} from "./protocol/motis/paxforecast";
import { sendRequest } from "./request";

export async function sendPaxForecastApplyMeasuresRequest(
  content: PaxForecastApplyMeasuresRequest
): Promise<PaxForecastApplyMeasuresResponse> {
  const msg = await sendRequest(
    "/paxforecast/apply_measures",
    "PaxForecastApplyMeasuresRequest",
    content
  );
  verifyContentType(msg, "PaxForecastApplyMeasuresResponse");
  return msg.content as PaxForecastApplyMeasuresResponse;
}
