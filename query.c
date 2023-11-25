#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

char *expandTilde(const char *path)
{
    if (path[0] == '~')
    {
        const char *home_dir = getenv("HOME");
        if (home_dir == NULL)
        {
            fprintf(stderr, "Error: HOME environment variable not set.\n");
            exit(EXIT_FAILURE);
        }

        size_t home_len = strlen(home_dir);
        size_t path_len = strlen(path);

        char *expanded_path = malloc(home_len + path_len);

        if (expanded_path == NULL)
        {
            fprintf(stderr, "Error: Memory allocation failed.\n");
            exit(EXIT_FAILURE);
        }

        strcpy(expanded_path, home_dir);
        strcpy(expanded_path + home_len, path + 1); // Skip the tilde

        return expanded_path;
    }
    else
    {
        return strdup(path); // Return a copy of the original path
    }
}

const char *getFileName(const char *path)
{
    // Find the last directory separator in the path
    const char *lastSeparator = strrchr(path, '/');

    // If '/' is not found, try '\'
    if (lastSeparator == NULL)
    {
        lastSeparator = strrchr(path, '\\');
    }

    // If neither '/' nor '\' is found, use the whole path
    if (lastSeparator == NULL)
    {
        return path;
    }

    // Return the substring after the last separator
    return lastSeparator + 1;
}

void copyFile(const char *sourcePath, const char *destinationDir)
{
    // Open the source file for reading
    FILE *sourceFile = fopen(expandTilde(sourcePath), "rb");
    if (sourceFile == NULL)
    {
        fprintf(stderr, "Error opening source file: %s\n", sourcePath);
        exit(EXIT_FAILURE);
    }

    // Construct the destination file path
    const char *sourceFileName = getFileName(sourcePath);
    char destinationPath[256]; // Adjust the size accordingly
    snprintf(destinationPath, sizeof(destinationPath), "%s/%s", destinationDir, sourceFileName);

    // Open the destination file for writing
    FILE *destinationFile = fopen(destinationPath, "wb");
    if (destinationFile == NULL)
    {
        perror("Error opening destination file");
        fclose(sourceFile);
        exit(EXIT_FAILURE);
    }

    // Copy data from source to destination
    char buffer[1024]; // Adjust the size accordingly
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0)
    {
        fwrite(buffer, 1, bytesRead, destinationFile);
    }

    // Close the files
    fclose(sourceFile);
    fclose(destinationFile);

    printf("File copied successfully from %s to %s\n", sourcePath, destinationPath);
}

int main()
{
    sqlite3 *db;
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT attachment.filename "
        "FROM handle "
        "JOIN chat_handle_join "
        "  ON chat_handle_join.handle_id = handle.ROWID "
        "JOIN chat "
        "  ON chat.ROWID = chat_handle_join.chat_id "
        "JOIN chat_message_join "
        "  ON chat_message_join.chat_id = chat.ROWID "
        "JOIN message "
        "  ON message.ROWID = chat_message_join.message_id "
        "JOIN message_attachment_join "
        "  ON message_attachment_join.message_id = message.ROWID "
        "JOIN attachment "
        "  ON attachment.ROWID = message_attachment_join.attachment_id "
        "WHERE "
        "     handle.uncanonicalized_id = '7177428193' "
        "AND "
        "     mime_type IS NOT NULL";
    int rc;

    // Open the SQLite database
    rc = sqlite3_open(expandTilde("~/Library/Messages/chat.db"), &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database (%i): %s\n", rc, sqlite3_errmsg(db));
        return 1;
    }

    // Prepare the SQL statement
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot prepare SQL statement: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Execute the query
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        // Process each row
        const char *textValue = (const char *)sqlite3_column_text(stmt, 0);

        if (textValue != NULL)
        {
            copyFile(textValue, "/tmp/7150");
        }
        else
        {
            printf("Text Value is NULL\n");
            return 1;
        }
    }

    // Finalize the statement and close the database
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}
