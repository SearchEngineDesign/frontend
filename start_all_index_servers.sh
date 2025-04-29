#!/bin/bash



# ./run_index_server.sh"
COMMAND="cd ../tkmaher/searchengine/frontend && pkill -f run_index_server.sh && ./run_indexserver.sh" 
project_id="decent-decker-456116-p8"



instances=(
    "realsearchclone1-20250421-022420"
    "realsearchclone10-20250427-145510"
    "realsearchclone2-20250421-155447"
    # "realsearchclone3-20250424-160427",
    # "realsearchclone4-20250425-144724",
    # "realsearchclone5-20250426-202253",
    # "realsearchclone6-20250426-202325",
    # "realsearchclone7-20250427-145613",
    # "realsearchclone8-20250427-145830",
    # "realsearchclone9-20250427-145944",
)

for instance in "${instances[@]}"; do
    server="gcloud compute ssh $instance --project $project_id --command='$COMMAND'"
    echo $server
    gnome-terminal -- bash -c "$server1; exec bash" & SSH_PIDS+=($!) # save the SSH client's PID
    sleep 0.2
done


echo "All SSH sessions started."

# Save SSH PIDs
echo "${SSH_PIDS[@]}" > ssh_pids.txt

echo "When you want to interrupt all SSH commands, run:"
echo "kill -SIGINT ${SSH_PIDS[@]}"