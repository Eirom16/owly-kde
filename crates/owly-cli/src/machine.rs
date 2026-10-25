//! A presentation adapter for graphical clients. The AirDrop driver still
//! owns discovery, request expiry, extraction and all transfer decisions.
use std::collections::HashMap;
use std::io::{self, Write};
use std::process::ExitCode;

use owly_airdrop::driver::receiver::{ReceiverEvent, Request};
use owly_airdrop::proto::messages::Offer;
use serde::Deserialize;
use serde_json::json;
use tokio::io::AsyncBufReadExt;
use tokio::sync::mpsc;

pub fn emit(value: serde_json::Value) {
    let mut out = io::stdout().lock();
    let _ = writeln!(out, "{value}");
    let _ = out.flush();
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Answer {
    id: u64,
    accept: bool,
}

pub async fn receive(mut events: mpsc::Receiver<ReceiverEvent>) -> io::Result<ExitCode> {
    let mut pending: HashMap<u64, Request> = HashMap::new();
    let mut stdin = tokio::io::BufReader::new(tokio::io::stdin()).lines();
    loop {
        tokio::select! {
            _ = tokio::signal::ctrl_c() => return Ok(ExitCode::SUCCESS),
            line = stdin.next_line() => {
                let Some(line) = line? else { return Ok(ExitCode::SUCCESS) };
                if let Ok(answer) = serde_json::from_str::<Answer>(&line)
                    && let Some(request) = pending.remove(&answer.id) {
                    if answer.accept { request.accept(); } else { request.decline(); }
                }
            }
            event = events.recv() => match event {
                None => return Ok(ExitCode::SUCCESS),
                Some(ReceiverEvent::Discovered { .. }) => {},
                Some(ReceiverEvent::Request(request)) => {
                    let items: Vec<_> = match &request.ask.offer {
                        Offer::Files(files) => files.iter().map(|f| json!({"name": f.name, "size": f.size})).collect(),
                        Offer::Urls(urls) => urls.iter().map(|u| json!({"name": u})).collect(),
                    };
                    emit(json!({"event": "request", "id": request.id, "sender": request.ask.sender_computer_name, "items": items}));
                    pending.insert(request.id, request);
                }
                Some(ReceiverEvent::Withdrawn { id }) => {
                    pending.remove(&id);
                    emit(json!({"event": "withdrawn", "id": id}));
                }
                Some(ReceiverEvent::Urls { id, urls }) => emit(json!({"event": "urls", "id": id, "urls": urls})),
                Some(ReceiverEvent::Received { id, paths }) => emit(json!({"event": "received", "id": id, "paths": paths})),
                Some(ReceiverEvent::Failed { id, reason }) => emit(json!({"event": "failed", "id": id, "reason": reason})),
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn decisions_require_an_explicit_boolean_and_request_id() {
        assert!(serde_json::from_str::<Answer>(r#"{"id":7,"accept":true}"#).is_ok());
        for invalid in [
            r#"{"accept":true}"#,
            r#"{"id":7}"#,
            r#"{"id":7,"accept":"yes"}"#,
            r#"{"id":7,"accept":true,"extra":1}"#,
        ] {
            assert!(serde_json::from_str::<Answer>(invalid).is_err());
        }
    }
}
