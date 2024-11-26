import os
import re as rgx
from itertools import chain
from requests import get, post
from requests import Response
from requests.exceptions import HTTPError, RequestException
import sys

# Constants
GITHUB_TOKEN=os.environ["GITHUB_TOKEN"]
GITHUB_API_URL=os.environ["GITHUB_API_URL"]
GITHUB_SERVER_URL=os.environ["GITHUB_SERVER_URL"]
GITHUB_SHA=os.environ["GITHUB_SHA"]

REPO_URL_PATH='DOI-USGS/ISIS3'
API_BASE_URL=f'{GITHUB_API_URL}/repos/{REPO_URL_PATH}'
API_PULLS_URL=f'{API_BASE_URL}/pulls'
API_COMMITS_URL=f'{API_BASE_URL}/commits'
API_ISSUES_URL=f'{API_BASE_URL}/issues'
ISSUES_URL=f'{GITHUB_SERVER_URL}/{REPO_URL_PATH}/issues/'
HEADERS = {
    "Accept": "application/vnd.github+json",
    "Authorization" : f"Bearer {GITHUB_TOKEN}",
    "X-GitHub-Api-Version": "2022-11-28"
}

def get_prs_associated_with_commit() -> Response:
    """
    Get list of PRs associated with commit.
    """
    try:
        response = get(f'{API_COMMITS_URL}/{GITHUB_SHA}/pulls', headers=HEADERS)
        response.raise_for_status()
        return response
    except HTTPError as he:
        raise HTTPError("HTTPError in retrieving list of PRs", he) 
    except RequestException as re:
        raise RequestException("Unable to retrieve list of PRs associated with commit.", re)

def get_pr_attributes(response: Response) -> tuple:
    """
    Get necessary PR attributes.
    """
    pull_response_json = response.json()
    if len(pull_response_json) == 0:
        # No PRs attributed to the commit
        print(False)
        sys.exit(0)
    pull_number = pull_response_json[0].get("number")
    pull_body = pull_response_json[0].get("body")
    return (pull_number, pull_body)

def search_for_linked_issues(pull_body: str) -> list:
    """
    Search for linked issues in PR body.
    Regex examples:
    '#1' - matches
    '#123456' - matches
    'https://github.com/DOI-USGS/ISIS3/issues/25' - matches
    '# 123' - fails
    '#ABC' - fails
    '## ABC'- fails
    """
    # Split the PR body by heading 
    pull_body_list = pull_body.split('##')
    regex_pattern = rf'{ISSUES_URL}(\d)|(#[^\D]\d*)'
    for section in pull_body_list:
        # Find section with heading 'Related Issue'
        if section != None and 'Related Issue' in section:
            # Find items that match the regex pattern
            matched_items = rgx.findall(regex_pattern, section)
            # Convert list of tuples to list of all items
            flattened_list = list(chain.from_iterable(matched_items))
            # Remove items of empty values
            filtered_list = list(filter(None, flattened_list))
            # Remove '#' from items
            issue_numbers = list(map(lambda item: item.replace('#', ''), filtered_list))
            return issue_numbers
    # No linked issues
    print(False)
    sys.exit(0)


def get_linked_issues(issue_numbers: list) -> list:
    """
    Verify issues exists and has labels.
    """
    response_list = []
    for issue_number in issue_numbers:
        # Get labels from issue
        try:
            response = get(f'{API_ISSUES_URL}/{issue_number}', headers=HEADERS)
            response.raise_for_status()
            response_list.append(response)
        except HTTPError as he:
            raise HTTPError("HTTPError in retrieving issues", he)
        except RequestException as re:
            raise RequestException("Unable to retrieve issues.", re)
    return response_list

def get_issue_labels(response_list: list) -> list:
    """
    Get labels from all linked issues.
    """
    combined_issue_labels = []
    for response in response_list:
        # Combine labels into a list
        issue_response_json = response.json()
        issue_labels = issue_response_json.get("labels")
        for issue_label in issue_labels:
            # Get name of each label object
            label_name = issue_label.get("name")
            if label_name not in combined_issue_labels:
                # Add label if it does not exist
                combined_issue_labels.append(label_name)
    if not combined_issue_labels:
        # No labels to return
        print(False)
        sys.exit(0)
    return combined_issue_labels

def update_pr_labels(pull_number: str, combined_issue_labels: list):
    """
    Update pull request with labels
    # Source: https://stackoverflow.com/q/68459601
    """
    # Convert label list into JSON-formatted dict
    labels_data = {"labels": combined_issue_labels}
    try:
        response = post(f'{API_ISSUES_URL}/{pull_number}/labels', json=labels_data, headers=HEADERS)
        response.raise_for_status()
    except HTTPError as he:
        raise HTTPError("HTTPError in updating PR.", he)
    except RequestException as re:
        raise RequestException("Unable to update PR.", re)

def get_pr(pull_number: str) -> Response:
    """
    Get pull request
    """
    try:
        response = get(f'{API_PULLS_URL}/{pull_number}', headers=HEADERS)
        response.raise_for_status()
        return response
    except HTTPError as he:
        raise HTTPError("HTTPError in retrieving issues", he)
    except RequestException as re:
        raise RequestException("Unable to retrieve issues.", re)

def is_pr_bugfix(response: Response) -> bool:
    """
    Check PR label for 'bug'
    """
    labels = response.json().get("labels")
    for label in labels:
        if label.get("name") == "bug":
            return True
    return False

if __name__ == "__main__":
    try:
        # Update PR labels
        response = get_prs_associated_with_commit()
        pull_number, pull_body = get_pr_attributes(response)
        issue_numbers = search_for_linked_issues(pull_body)
        response_list = get_linked_issues(issue_numbers)
        combined_issue_labels = get_issue_labels(response_list)
        update_pr_labels(pull_number, combined_issue_labels)

        # Check if PR is a bugfix
        response = get_pr(pull_number)
        print(is_pr_bugfix(response))
    except (HTTPError, RequestException):
        raise

