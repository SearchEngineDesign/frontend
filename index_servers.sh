#!/bin/bash


usage() {
    echo "Usage: $0 [start | stop]"
}



COMMAND="\"cd ../tkmaher/searchengine/frontend && pkill -f indexserver && pkill -SIGINT -f run_indexserver.sh && ./run_indexserver.sh && exit\""
project_id=decent-decker-456116-p8

CLOSE_COMMAND="\"cd ../tkmaher/searchengine/frontend && pkill -SIGINT -f run_indexserver.sh && exit\""


TMP_FILE=$(mktemp)

instances=(
    "realsearchclone1-20250421-022420"
    "realsearchclone10-20250427-145510"
    # "realsearchclone2-20250421-155447"
    # "realsearchclone3-20250424-160427"
    # "realsearchclone4-20250425-144724"
    # "realsearchclone5-20250426-202253"
    # "realsearchclone6-20250426-202325"
    # "realsearchclone7-20250427-145613"
    # "realsearchclone8-20250427-145830"
    # "realsearchclone9-20250427-145944"
)

case "$1" in
    "start")
        echo "Starting index servers..."

        for instance in "${instances[@]}"; do
            server="gcloud compute ssh $instance --project $project_id --command=$COMMAND"
            gnome-terminal -- bash -c " $server; exec bash" & TERMINAL_PID=$!
            echo $server
        done

        echo "Output from gnome-terminal command:"
        cat "$TMP_FILE"

        echo "All SSH sessions started."
        ;;
    "stop")
    echo "Stopping index servers..."
        for instance in "${instances[@]}"; do
            server="gcloud compute ssh $instance --project $project_id --command=$CLOSE_COMMAND"
            gnome-terminal -- bash -c "$server; exit; exec bash" & TERMINAL_PID=$!
            echo $server
        done

        echo "Output from gnome-terminal command:"
        cat "$TMP_FILE"

        echo "All SSH sessions stopped."
        ;;
    *)
        usage
        exit 1
esac







